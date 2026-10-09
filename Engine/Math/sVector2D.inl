#pragma once
#include "sVector2D.h"

namespace eae6320::Math
{
	// Interface
	//==========

	// Initialization / Clean Up
	//--------------------------

	constexpr sVector2D::sVector2D(float i_x, float i_y)
		: x(i_x), y(i_y)
	{
	}

	inline constexpr sVector2D::sVector2D(const sVector2D& i_toCopy)
		: sVector2D(i_toCopy.x, i_toCopy.y)
	{
	}

	inline sVector2D& sVector2D::operator=(const sVector2D& i_toCopy)
	{
		x = i_toCopy.x;
		y = i_toCopy.y;
		return *this;
	}

	// Operators
	//----------

	inline constexpr sVector2D sVector2D::operator+(const sVector2D& i_rhs) const
	{
		return sVector2D(x + i_rhs.x, y + i_rhs.y);
	}

	inline constexpr sVector2D& sVector2D::operator+=(const sVector2D& i_rhs)
	{
		x += i_rhs.x;
		y += i_rhs.y;
		return *this;
	}

	inline constexpr sVector2D sVector2D::operator+(float i_rhs) const
	{
		return sVector2D(x + i_rhs, y + i_rhs);
	}

	inline constexpr sVector2D& sVector2D::operator+=(float i_rhs)
	{
		x += i_rhs;
		y += i_rhs;
		return *this;
	}

	inline constexpr sVector2D sVector2D::operator-(const sVector2D& i_rhs) const
	{
		return sVector2D(x - i_rhs.x, y - i_rhs.y);
	}

	inline constexpr sVector2D& sVector2D::operator-=(const sVector2D& i_rhs)
	{
		x -= i_rhs.x;
		y -= i_rhs.y;
		return *this;
	}

	inline constexpr sVector2D sVector2D::operator-() const
	{
		return sVector2D(-x, -y);
	}

	inline constexpr sVector2D sVector2D::operator-(float i_rhs) const
	{
		return sVector2D(x - i_rhs, y - i_rhs);
	}

	inline constexpr sVector2D& sVector2D::operator-=(float i_rhs)
	{
		x -= i_rhs;
		y -= i_rhs;
		return *this;
	}

	inline constexpr sVector2D sVector2D::operator*(float i_rhs) const
	{
		return sVector2D(x * i_rhs, y * i_rhs);
	}

	inline constexpr sVector2D& sVector2D::operator*=(float i_rhs)
	{
		x *= i_rhs;
		y *= i_rhs;
		return *this;
	}

	// Comparison
	//-----------

	inline constexpr bool sVector2D::operator==(const sVector2D& i_rhs) const
	{
		return x == i_rhs.x && y == i_rhs.y;
	}

	inline constexpr bool sVector2D::operator!=(const sVector2D& i_rhs) const
	{
		return !(*this == i_rhs);
	}

	// Friends
	//========

	inline constexpr sVector2D operator+(float i_lhs, const sVector2D& i_rhs)
	{
		return sVector2D(i_lhs + i_rhs.x, i_lhs + i_rhs.y);
	}

	constexpr sVector2D operator-(float i_lhs, const sVector2D& i_rhs)
	{
		return sVector2D(i_lhs - i_rhs.x, i_lhs - i_rhs.y);
	}

	constexpr sVector2D operator*(float i_lhs, const sVector2D& i_rhs)
	{
		return sVector2D(i_lhs * i_rhs.x, i_lhs * i_rhs.y);
	}
}