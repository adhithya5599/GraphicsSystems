// Includes
//=========

#include "Functions.h"

#include <cmath>
#include <cstring>
#include <intrin.h>
#include <Engine/Asserts/Asserts.h>

#define CRC_DIVISOR 0xdeadbeef
#if (CRC_DIVISOR & 1) == 0
#error "Divisor's LSB must be 1 for CRC to work"
#endif

// Interface
//==========

struct sGroupView
{
	uintptr_t currentGroup = 0;
	uintptr_t nextGroup = 0;

	inline uint64_t* As64()
	{
		return reinterpret_cast<uint64_t*>(this);
	}

	void ModifyCurrent(uint64_t i_modifier)
	{
		*As64() ^= i_modifier;
	}

	void ModifyMiddle(uint64_t i_modifier)
	{
		constexpr bool smallGroupView = sizeof(sGroupView) == sizeof(uint64_t);
		if (smallGroupView)
		{
			ModifyCurrent(i_modifier);
		}
		else
		{
			constexpr size_t halfSize = sizeof(uintptr_t) * 8 / 2;
			uintptr_t currentModifier = static_cast<uintptr_t>(i_modifier << halfSize);
			uintptr_t nextModifier = static_cast<uintptr_t>(i_modifier >> halfSize);

			currentGroup ^= currentModifier;
			nextGroup ^= nextModifier;
		}
	}

	inline void* Middle()
	{
		return reinterpret_cast<char*>(this) + sizeof(uint32_t);
	}

	void Shift(uintptr_t i_newGroup)
	{
		currentGroup = nextGroup;
		nextGroup = i_newGroup;
	}
};

uint16_t eae6320::Math::ConvertFloatToHalf( const float i_value )
{
	// Get the individual bits of the floating point value
	const auto bitRepresentation = [i_value]
	{
		uint32_t bitRepresentation;
		{
			EAE6320_ASSERT( sizeof( bitRepresentation ) == sizeof( i_value ) );
			memcpy( &bitRepresentation, &i_value, sizeof( bitRepresentation ) );
		}
		return bitRepresentation;
	}();
	// The first bit is the sign
	const auto sign_float = bitRepresentation & 0x80000000;
	const auto sign_half = static_cast<uint16_t>( sign_float >> 16 );
	// The next 8 bits are the exponent
	// (a half has 5 bits)
	const auto exponent_float = bitRepresentation & 0x7f800000;
	// The final 23 bits are the significand
	// (a half has 10 bits)
	const auto significand_float = bitRepresentation & 0x7fffff;
	// Check for values that are out of range
	const auto absoluteValue_float = bitRepresentation & 0x7fffffff;
	constexpr uint32_t maxHalfValue_float = 0x477FE000;	// Exponent == 15, Significand = 0b1111111111 << (23 - 10)
	if ( absoluteValue_float <= maxHalfValue_float )
	{
		constexpr uint32_t minNormalHalfValue_float = 0x38800000;	// Exponent == -14
		if ( absoluteValue_float >= minNormalHalfValue_float )
		{
			const auto value_half = static_cast<uint16_t>(
				// Shift the value to line up with the half bits
				// (this will truncate the significand)
				(absoluteValue_float >> 13)	// 23 - 10
				// Unbias the float exponent by subtracting 127 and then rebias for half precision by adding 15
				- 0x1c000);	// (127 - 15) << 10
			return value_half;
		}
		else
		{
			constexpr uint32_t minSubNormalHalfValue_float = 0x33800000;
			if ( absoluteValue_float < minSubNormalHalfValue_float )
			{
				// If the value is too small to be represented by a half it is set to zero
				return sign_half;
			}
			else
			{
				// The value is too small to be represented by normal half bit patterns,
				// and so it must be converted to a subnormal representation
				// (Warning! This code is tricky, and I'm not certain that it is correct)

				// Instead of an implicit leading 1 make it explicit
				const auto explicitSignificand_float = significand_float | 0x800000;
				// Half precision denormals always have an exponent of -14,
				// and so the significand must be shifted accordingly
				const auto significand_half = static_cast<uint16_t>(
					explicitSignificand_float >> ( ( 127 - ( exponent_float >> 23 ) ) - 1 ) );
				return sign_half | significand_half;
			}
		}
	}
	else
	{
		// The value is either too large to be represented as a half
		// or it is a NaN
		const auto isNaN =
			// Is the exponent 0xff?
			( exponent_float == 0x7f800000 )
			// If the exponent is 0xff and the significand is zero then the value is infinity.
			// Any other significand value is a NaN.
			&& ( significand_float != 0 );
		if ( !isNaN )
		{
			// If the value is too large then return +/- infinity
			constexpr uint16_t infinity_half = 0x7c00;
			return sign_half | infinity_half;
		}
		else
		{
			// The first bit of the NaN significand determines whether is quiet (vs. signaling)
			const auto isQuiet_float = significand_float & 0x400000;
			const uint16_t isQuiet_half = isQuiet_float ? 0x200 : 0;
			// The remaining bits are the NaN "payload"
			const auto payload_float = significand_float & 0x3fffff;
			// There is no standard of what the payload means,
			// and so it is simply truncated.
			const auto payload_half = static_cast<uint16_t>( significand_float & 0x1ff );
			return sign_half | isQuiet_half | payload_half;
		}
	}
}

float eae6320::Math::ConvertHorizontalFieldOfViewToVerticalFieldOfView( const float i_horizontalFieldOfView_inRadians,
	const float i_aspectRatio )
{
	return 2.0f * std::atan( std::tan( i_horizontalFieldOfView_inRadians * 0.5f ) / i_aspectRatio );
}

// (CameraControls new operations)
//================================

uint32_t eae6320::Math::HashCRC32(const void* i_data, size_t i_size)
{
	if (i_size == 0) return 0;

	size_t sizeRemainder = i_size % sizeof(uintptr_t);
	size_t lastGroupSize = sizeRemainder == 0 ? sizeof(uintptr_t) : sizeRemainder;
	size_t numGroups = i_size / sizeof(uintptr_t) + (sizeRemainder != 0 ? 1 : 0);

	sGroupView currentGroups;
	const uintptr_t* dataGroups = static_cast<const uintptr_t*>(i_data);

	// Initialize the first two groups, making sure not to read invalid data
	if (numGroups == 1 && sizeRemainder != 0)
	{
		memcpy(&currentGroups.currentGroup, &dataGroups[0], sizeRemainder);
	}
	else
	{
		currentGroups.currentGroup = dataGroups[0];
	}

	if (numGroups == 1)
	{
		currentGroups.nextGroup = 0;
	}
	else if (numGroups == 2 && sizeRemainder != 0)
	{
		memcpy(&currentGroups.nextGroup, &dataGroups[1], sizeRemainder);
	}
	else
	{
		currentGroups.nextGroup = dataGroups[1];
	}

#ifdef _DEBUG
	int numIterationsInGroup = 0;
#endif

	for (size_t currentGroupIndex = 0; ;)
	{
		unsigned long earliestOneBit = 0;
		bool currentGroupContainsOne = false;

		// Find the least significant one bit in this group
#ifdef _WIN64
		currentGroupContainsOne = _BitScanForward64(&earliestOneBit, currentGroups.currentGroup);
#elif defined(_WIN32)
		currentGroupContainsOne = _BitScanForward(&earliestOneBit, currentGroups.currentGroup);
#else
		// Manually find the first bit
		for (size_t currentGroup = currentGroups.currentGroup; currentGroup != 0; currentGroup >>= 1)
		{
			if (currentGroup & 1)
			{
				currentGroupContainsOne = true;
				break;
			}

			earliestOneBit++;
		}
#endif

		// If we're on the last group and no more bits in the group's remaining bytes
		if (currentGroupIndex == numGroups - 1 && (!currentGroupContainsOne || earliestOneBit / 8 >= lastGroupSize))
		{
			break;
		}

		// If no more one bits in group, go to next one
		if (!currentGroupContainsOne)
		{
			currentGroupIndex++;

			if (currentGroupIndex < numGroups - 1)
			{
				// Store the next group into currentGroups
				if (currentGroupIndex == numGroups - 2 && sizeRemainder != 0)
				{
					// If next group is the last group but it's not a complete group;
					// read only sizeRemainder bytes to avoid reading invalid data
					uintptr_t lastGroup = 0;
					memcpy(&lastGroup, &dataGroups[currentGroupIndex + 1], sizeRemainder);
					currentGroups.Shift(lastGroup);
				}
				else
				{
					currentGroups.Shift(dataGroups[currentGroupIndex + 1]);
				}
			}
			else if (currentGroupIndex == numGroups - 1)
			{
				// Current group is the last group, store 0 in the next group
				currentGroups.Shift(0);
			}

#ifdef _DEBUG
			numIterationsInGroup = 0;
#endif
			continue;
		}
#ifdef _DEBUG
		else if (++numIterationsInGroup > sizeof(size_t) * 8)
		{
			//EAE6320_FAIL("Encountered infinite loop in HashCRC32");
		}
#endif

		constexpr unsigned int numBitsInDivisor = sizeof(uint32_t) * 8;
		bool isUpper = earliestOneBit >= numBitsInDivisor;
		if (isUpper)
		{
			earliestOneBit -= numBitsInDivisor;
			uint64_t modifier = static_cast<uint64_t>(CRC_DIVISOR) << earliestOneBit;
			currentGroups.ModifyMiddle(modifier);
		}
		else
		{
			uint64_t modifier = static_cast<uint64_t>(CRC_DIVISOR) << earliestOneBit;
			currentGroups.ModifyCurrent(modifier);
		}
	}

	uint32_t hash = static_cast<uint32_t>(currentGroups.currentGroup >> (lastGroupSize * 8));
	hash |= static_cast<uint32_t>(currentGroups.nextGroup << ((sizeof(uintptr_t) - lastGroupSize) * 8));
	return hash;
}

// (end CameraControls new operations)
