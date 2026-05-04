/*
	This struct represents a three-dimensional position or direction
*/

#ifndef EAE6320_MATH_SVECTOR2_H
#define EAE6320_MATH_SVECTOR2_H

// Struct Declaration
//===================

namespace eae6320
{
	namespace Math
	{
		struct sVector;
		struct sVector2
		{
			// Data
			//=====

			float x = 0.0f, y = 0.0f;

			// Interface
			//==========

			// Addition
			//---------

			constexpr sVector2 operator +(const sVector2& i_rhs) const;
			constexpr sVector2& operator +=(const sVector2& i_rhs);
			constexpr volatile sVector2& operator +=(const volatile sVector2& i_rhs);

			constexpr sVector2 operator +(const float i_rhs) const;
			constexpr sVector2& operator +=(const float i_rhs);
			friend constexpr sVector2 operator +(const float i_lhs, const sVector2& i_rhs);


			// Subtraction / Negation
			//-----------------------

			constexpr sVector2 operator -(const sVector2& i_rhs) const;
			constexpr sVector2& operator -=(const sVector2& i_rhs);
			constexpr sVector2 operator -() const;

			constexpr sVector2 operator -(const float i_rhs) const;
			constexpr sVector2& operator -=(const float i_rhs);
			friend constexpr sVector2 operator -(const float i_lhs, const sVector2& i_rhs);

			sVector2& operator =(const sVector& i_other);

			// Products
			//---------

			constexpr sVector2 operator *(const float i_rhs) const;
			constexpr sVector2& operator *=(const float i_rhs);
			friend constexpr sVector2 operator *(const float i_lhs, const sVector2& i_rhs);

			friend constexpr float Dot(const sVector2& i_lhs, const sVector2& i_rhs);
			friend constexpr float Cross(const sVector2& i_lhs, const sVector2& i_rhs);

			// Division
			//---------

			sVector2 operator /(const float i_rhs) const;
			sVector2& operator /=(const float i_rhs);

			// Length / Normalization
			//-----------------------

			float GetLength() const;
			float Normalize();
			float Distance(const sVector2& i_other) const;
			sVector2 GetNormalized() const;
			float GetLengthSquared() const;

			// Comparison
			//-----------

			constexpr bool operator ==(const sVector2& i_rhs) const;
			constexpr bool operator !=(const sVector2& i_rhs) const;

			// Initialization / Clean Up
			//--------------------------

			constexpr sVector2() = default;
			constexpr sVector2(const float i_x, const float i_y);

			static sVector2 Zero() { return sVector2(0.f, 0.f); }
		};

		// Friends
		//========

		constexpr sVector2 operator +(const float i_lhs, const sVector2& i_rhs);
		constexpr sVector2 operator -(const float i_lhs, const sVector2& i_rhs);
		constexpr sVector2 operator *(const float i_lhs, const sVector2& i_rhs);
		constexpr float Dot(const sVector2& i_lhs, const sVector2& i_rhs);
		constexpr float Cross(const sVector2& i_lhs, const sVector2& i_rhs);
	}
}

#include "sVector2.inl"

#endif	// EAE6320_MATH_SVECTOR_H
