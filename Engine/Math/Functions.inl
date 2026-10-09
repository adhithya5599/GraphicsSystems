#ifndef EAE6320_MATH_FUNCTIONS_INL
#define EAE6320_MATH_FUNCTIONS_INL

// Includes
//=========

#include "Functions.h"

#include "Constants.h"

#include <cmath>
#include <Engine/Asserts/Asserts.h>

// Interface
//==========

constexpr float eae6320::Math::ConvertDegreesToRadians( const float i_degrees )
{
	return i_degrees * ( g_pi / 180.0f );
}

	template<typename tUnsignedInteger, class EnforceUnsigned>
tUnsignedInteger eae6320::Math::RoundUpToMultiple( const tUnsignedInteger i_value, const tUnsignedInteger i_multiple )
{
	EAE6320_ASSERTF( i_multiple != 0, "Zero isn't a valid multiple" );
	EAE6320_ASSERTF( i_multiple > 0, "The multiple must be positive" );
	// Adding one less than the multiple will make the value at or above the next hiehst multiple
	// unless the value was itself a multiple.
	// Dividng and multiplying by the multiple removes any remainder
	const auto returnValue = ( ( i_value + i_multiple - 1 ) / i_multiple ) * i_multiple;
	EAE6320_ASSERT( ( returnValue % i_multiple ) == 0 );
	return returnValue;
}

	template<typename tUnsignedInteger, class EnforceUnsigned>
tUnsignedInteger eae6320::Math::RoundUpToMultiple_powerOf2( const tUnsignedInteger i_value, const tUnsignedInteger i_multipleWhichIsAPowerOf2 )
{
	EAE6320_ASSERTF( i_multipleWhichIsAPowerOf2 != 0, "Zero isn't a valid multiple" );
	EAE6320_ASSERTF( i_multipleWhichIsAPowerOf2 > 0, "The multiple must be positive" );
	// To be a power-of-2 the multiple can only have a single bit set;
	// get a mask of the bits less-significant than that single bit
	const auto nonLeadingBits = i_multipleWhichIsAPowerOf2 - 1;
	EAE6320_ASSERTF( ( i_multipleWhichIsAPowerOf2 && ( i_multipleWhichIsAPowerOf2 & nonLeadingBits ) ) == 0, "The multiple must be a power-of-2" );
	// Adding the non-leading bits will make the value at or above the next highest multiple
	// unless the value was itself a multiple.
	// ANDing with inverse then removes any bits less than the multiple.
	const auto returnValue = ( i_value + nonLeadingBits ) & ~nonLeadingBits;
	EAE6320_ASSERT( ( returnValue % i_multipleWhichIsAPowerOf2 ) == 0 );
	return returnValue;
}

// (CameraControls new operations)
//================================

constexpr float eae6320::Math::FastNegativeExp(float i_x)
{
	return 1.f / (1.f + i_x + (0.48f * i_x * i_x) + (0.235f * i_x * i_x * i_x));
}

// Comparison
//-----------

constexpr bool eae6320::Math::Approximately(float i_lhs, float i_rhs, float i_epsilon)
{
	float difference = i_lhs - i_rhs;
	float absDifference = difference < 0 ? -difference : difference;
	return absDifference < i_epsilon;
}

// Lerp
//-----

constexpr float eae6320::Math::Lerp(float i_from, float i_to, float i_time)
{
	return LerpUnclamped(i_from, i_to, Clamp(i_time, 0, 1));
}

constexpr float eae6320::Math::LerpUnclamped(float i_from, float i_to, float i_time)
{
	return i_from + (i_to - i_from) * i_time;
}

// Clamp
//------

constexpr float eae6320::Math::Clamp(float i_value, float i_min, float i_max)
{
	if (i_value < i_min)
	{
		return i_min;
	}
	else if (i_value > i_max)
	{
		return i_max;
	}
	return i_value;
}

// Damp
//-----

constexpr float eae6320::Math::Damp(float i_from, float i_to, float i_halflife, float i_deltaSeconds)
{
	// https://github.com/AlexisBacot/ArtOfDamping/blob/main/Assets/Scripts/ToolDamper.cs
	float exp = (0.69314718056f * i_deltaSeconds) / (i_halflife + std::numeric_limits<float>::epsilon());
	float lerpTime = 1.f - FastNegativeExp(exp);
	return Lerp(i_from, i_to, lerpTime);
}

constexpr float eae6320::Math::SmoothDamp(float i_from, float i_to, float i_smoothTime, float i_deltaSeconds, float& io_velocity)
{
	// https://github.com/AlexisBacot/ArtOfDamping/blob/main/Assets/Scripts/ToolDamper.cs
	// also from Game Programming Gems 4, Chapter 1.10
	float omega = 2.f / i_smoothTime;
	float x = omega * i_deltaSeconds;
	float exp = FastNegativeExp(x);

	float change = i_from - i_to;
	float temp = (io_velocity + omega * change) * i_deltaSeconds;
	io_velocity = (io_velocity - omega * temp) * exp;

	return i_to + (change + temp) * exp;
}

// (end CameraControls new operations)

#endif	// EAE6320_MATH_FUNCTIONS_INL