#pragma once
#include "sRectangle.h"

namespace eae6320::Math
{
	// Interface
	//==========
	
	// Initialization / Clean Up
	//--------------------------

	constexpr sRectangle::sRectangle(float i_left, float i_right, float i_bottom, float i_top)
		: left(i_left), right(i_right), bottom(i_bottom), top(i_top)
	{
	}

	constexpr sRectangle::sRectangle(const sVector2D& bottomLeft, const sVector2D& topRight)
		: sRectangle(bottomLeft.x, topRight.x, bottomLeft.y, topRight.y)
	{
	}

	// Operators
	//----------

	inline constexpr sRectangle sRectangle::operator+(const sVector2D& i_rhs) const
	{
		return sRectangle(left + i_rhs.x, right + i_rhs.x, bottom + i_rhs.y, top + i_rhs.y);
	}

	inline constexpr sRectangle& sRectangle::operator+=(const sVector2D& i_rhs)
	{
		left += i_rhs.x;
		right += i_rhs.x;
		bottom += i_rhs.y;
		top += i_rhs.y;

		return *this;
	}

	inline constexpr sRectangle sRectangle::operator-(const sVector2D& i_rhs) const
	{
		return sRectangle(left - i_rhs.x, right - i_rhs.x, bottom - i_rhs.y, top - i_rhs.y);
	}

	inline constexpr sRectangle& sRectangle::operator-=(const sVector2D& i_rhs)
	{
		left -= i_rhs.x;
		right -= i_rhs.x;
		bottom -= i_rhs.y;
		top -= i_rhs.y;

		return *this;
	}

	// Corners
	//--------

	constexpr sVector2D sRectangle::BottomLeft() const
	{
		return sVector2D(left, bottom);
	}

	constexpr sVector2D sRectangle::BottomRight() const
	{
		return sVector2D(right, bottom);
	}

	constexpr sVector2D sRectangle::TopLeft() const
	{
		return sVector2D(left, top);
	}

	constexpr sVector2D sRectangle::TopRight() const
	{
		return sVector2D(right, top);
	}

	// Friends
	//========

	inline constexpr sRectangle operator+(const sVector2D& i_lhs, const sRectangle& i_rhs)
	{
		return i_rhs + i_lhs;
	}

	inline constexpr sRectangle operator-(const sVector2D& i_lhs, const sRectangle& i_rhs)
	{
		return i_rhs + i_lhs;
	}
}