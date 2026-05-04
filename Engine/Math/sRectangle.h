#pragma once
#include "sVector2D.h"

// Struct Declaration
//===================

namespace eae6320::Math
{
	struct sRectangle
	{
		// Data
		//=====

		float left = 0.f;
		float right = 0.f;
		float bottom = 0.f;
		float top = 0.f;

		// Interface
		//==========

		// Initialization / Clean Up
		//--------------------------

		sRectangle() = default;
		constexpr sRectangle(float i_left, float i_right, float i_bottom, float i_top);
		constexpr sRectangle(const sVector2D& bottomLeft, const sVector2D& topRight);

		// Operators
		//----------

		constexpr sRectangle operator+(const sVector2D& i_rhs) const;
		constexpr sRectangle& operator+=(const sVector2D& i_rhs);
		friend constexpr sRectangle operator+(const sVector2D& i_lhs, const sRectangle& i_rhs);

		constexpr sRectangle operator-(const sVector2D& i_rhs) const;
		constexpr sRectangle& operator-=(const sVector2D& i_rhs);
		friend constexpr sRectangle operator-(const sVector2D& i_lhs, const sRectangle& i_rhs);

		// Corners
		//--------

		constexpr sVector2D BottomLeft() const;
		constexpr sVector2D BottomRight() const;
		constexpr sVector2D TopLeft() const;
		constexpr sVector2D TopRight() const;
	};

	// Friends
	//========

	constexpr sRectangle operator+(const sVector2D& i_lhs, const sRectangle& i_rhs);
	constexpr sRectangle operator-(const sVector2D& i_lhs, const sRectangle& i_rhs);
}

#include "sRectangle.inl"