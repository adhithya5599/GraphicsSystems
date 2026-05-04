#pragma once
#include <limits>

// Struct Declaration
//===================

namespace eae6320::Math
{
	struct sVector2D
	{
		// Data
		//=====

		float x = 0.f;
		float y = 0.f;

		// Constants
		//----------

		static const sVector2D Zero;
		static const sVector2D Left;
		static const sVector2D Right;
		static const sVector2D Up;
		static const sVector2D Down;
		static const sVector2D One;

		// Interface
		//==========
		
		// Initialization / Clean Up
		//--------------------------

		constexpr sVector2D() = default;
		constexpr sVector2D(float i_x, float i_y);
		constexpr sVector2D(const sVector2D& i_toCopy);

		sVector2D& operator=(const sVector2D& i_toCopy);

		// Operators
		//----------

		constexpr sVector2D operator+(const sVector2D& i_rhs) const;
		constexpr sVector2D& operator+=(const sVector2D& i_rhs);
		friend constexpr sVector2D operator+(float i_lhs, const sVector2D& i_rhs);

		constexpr sVector2D operator+(float i_rhs) const;
		constexpr sVector2D& operator+=(float i_rhs);

		constexpr sVector2D operator-(const sVector2D& i_rhs) const;
		constexpr sVector2D& operator-=(const sVector2D& i_rhs);
		friend constexpr sVector2D operator-(float i_lhs, const sVector2D& i_rhs);
		constexpr sVector2D operator-() const;

		constexpr sVector2D operator-(float i_rhs) const;
		constexpr sVector2D& operator-=(float i_rhs);

		constexpr sVector2D operator*(float i_rhs) const;
		constexpr sVector2D& operator*=(float i_rhs);
		friend constexpr sVector2D operator*(float i_lhs, const sVector2D& i_rhs);

		sVector2D operator/(float i_rhs) const;
		sVector2D& operator/=(float i_rhs);

		// Comparison
		//-----------

		constexpr bool operator==(const sVector2D& i_rhs) const;
		constexpr bool operator!=(const sVector2D& i_rhs) const;

		static bool Approximately(const sVector2D& i_lhs, const sVector2D& i_rhs, float i_epsilon = std::numeric_limits<float>::epsilon());

		// Operations
		//-----------

		float SqrMagnitude() const;
		float Magnitude() const;

		sVector2D Normalized() const;
		void Normalize();
	};

	// Friends
	//========

	float Dot(const sVector2D& i_lhs, const sVector2D& i_rhs);

	constexpr sVector2D operator+(float i_lhs, const sVector2D& i_rhs);
	constexpr sVector2D operator-(float i_lhs, const sVector2D& i_rhs);
	constexpr sVector2D operator*(float i_lhs, const sVector2D& i_rhs);
}

#include "sVector2D.inl"