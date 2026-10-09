/*
	This file contains math-related functions
*/

#ifndef EAE6320_MATH_FUNCTIONS_H
#define EAE6320_MATH_FUNCTIONS_H

#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>
#include <Engine/Math/sVector2.h>
#include <cmath>

namespace eae6320
{
	namespace Math
	{

		// Interface
		//==========

		// The engine uses radians internally, but this function can be used to convert from degrees
		constexpr float ConvertDegreesToRadians( const float i_degrees );
		// Converts from standard 32 bit floats to 16 bit floats
		// (where the bit representation is returned as a uint16_t since C++ doesn't support them natively)
		uint16_t ConvertFloatToHalf( const float i_value );
		// cMatrix_transformation::CreateCameraToProjectedTransform_perspective() expects a vertical FOV,
		// but a horizontal FOV can be converted to vertical using this function
		float ConvertHorizontalFieldOfViewToVerticalFieldOfView( const float i_horizontalFieldOfView_inRadians,
			// aspectRatio = width / height
			const float i_aspectRatio );

		// Rounds up an unsigned integer to the next highest multiple
			template<typename tUnsignedInteger, class EnforceUnsigned = typename std::enable_if_t<std::is_unsigned<tUnsignedInteger>::value>>
		tUnsignedInteger RoundUpToMultiple( const tUnsignedInteger i_value, const tUnsignedInteger i_multiple );
		// If the multiple is known to be a power-of-2 this is cheaper than the previous function
			template<typename tUnsignedInteger, class EnforceUnsigned = typename std::enable_if_t<std::is_unsigned<tUnsignedInteger>::value>>
		tUnsignedInteger RoundUpToMultiple_powerOf2( const tUnsignedInteger i_value, const tUnsignedInteger i_multipleWhichIsAPowerOf2 );

		// (CameraControls new operations)
		//================================
		
		// CRC (32-bit) hash function: https://en.wikipedia.org/wiki/Cyclic_redundancy_check
		uint32_t HashCRC32(const void* i_data, size_t i_size);

		constexpr float FastNegativeExp(float i_num);

		// Comparison
		//-----------

		constexpr bool Approximately(float i_lhs, float i_rhs, float i_epsilon = std::numeric_limits<float>::epsilon());

		// Lerp
		//-----

		constexpr float Lerp(float i_from, float i_to, float i_time);
		constexpr float LerpUnclamped(float i_from, float i_to, float i_time);

		// Clamp
		//------

		constexpr float Clamp(float i_value, float i_min, float i_max);

		// Damping
		//--------

		constexpr float Damp(float i_from, float i_to, float i_halflife, float i_deltaSeconds);
		constexpr float SmoothDamp(float i_from, float i_to, float i_smoothTime, float i_deltaSeconds, float& io_velocity);

		// (end CameraControls new operations)

		//Physics
		//--------
		constexpr float VerySmallFloat = 0.0005f;
        inline bool NearlyEqual(float i_a, float i_b)
        {
			return std::fabs(i_a - i_b) < VerySmallFloat;
        }            

		inline bool NearlyEqual(const sVector2& i_a, const sVector2& i_b)
		{
			return i_a.Distance(i_b) < VerySmallFloat;
		}
	}
}

#include "Functions.inl"

#endif	// EAE6320_MATH_FUNCTIONS_H
