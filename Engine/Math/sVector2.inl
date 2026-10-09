#ifndef EAE6320_MATH_SVECTOR2_INL
#define EAE6320_MATH_SVECTOR2_INL

// Includes
//=========

#include "sVector2.h"


// Interface
//==========

// Addition
//---------

constexpr eae6320::Math::sVector2 eae6320::Math::sVector2::operator +(const sVector2& i_rhs) const
{
	return sVector2(x + i_rhs.x, y + i_rhs.y);
}

constexpr eae6320::Math::sVector2& eae6320::Math::sVector2::operator +=(const sVector2& i_rhs)
{
	x += i_rhs.x;
	y += i_rhs.y;
	return *this;
}

constexpr volatile eae6320::Math::sVector2& eae6320::Math::sVector2::operator +=(const volatile sVector2& i_rhs)
{
	x += i_rhs.x;
	y += i_rhs.y;
	return *this;
}

constexpr eae6320::Math::sVector2 eae6320::Math::sVector2::operator +(const float i_rhs) const
{
	return sVector2(x + i_rhs, y + i_rhs);
}

constexpr eae6320::Math::sVector2& eae6320::Math::sVector2::operator +=(const float i_rhs)
{
	x += i_rhs;
	y += i_rhs;
	return *this;
}

constexpr eae6320::Math::sVector2 eae6320::Math::operator +(const float i_lhs, const sVector2& i_rhs)
{
	return i_rhs + i_lhs;
}

// Subtraction / Negation
//-----------------------

constexpr eae6320::Math::sVector2 eae6320::Math::sVector2::operator -(const sVector2& i_rhs) const
{
	return sVector2(x - i_rhs.x, y - i_rhs.y);
}

constexpr eae6320::Math::sVector2& eae6320::Math::sVector2::operator -=(const sVector2& i_rhs)
{
	x -= i_rhs.x;
	y -= i_rhs.y;
	return *this;
}

constexpr eae6320::Math::sVector2 eae6320::Math::sVector2::operator -() const
{
	return sVector2(-x, -y);
}

constexpr eae6320::Math::sVector2 eae6320::Math::sVector2::operator -(const float i_rhs) const
{
	return sVector2(x - i_rhs, y - i_rhs);
}

constexpr eae6320::Math::sVector2& eae6320::Math::sVector2::operator -=(const float i_rhs)
{
	x -= i_rhs;
	y -= i_rhs;
	return *this;
}

constexpr eae6320::Math::sVector2 eae6320::Math::operator -(const float i_lhs, const sVector2& i_rhs)
{
	return sVector2(i_lhs - i_rhs.x, i_lhs - i_rhs.y);
}

// Products
//---------

constexpr eae6320::Math::sVector2 eae6320::Math::sVector2::operator *(const float i_rhs) const
{
	return sVector2(x * i_rhs, y * i_rhs);
}

constexpr eae6320::Math::sVector2& eae6320::Math::sVector2::operator *=(const float i_rhs)
{
	x *= i_rhs;
	y *= i_rhs;
	return *this;
}

constexpr eae6320::Math::sVector2 eae6320::Math::operator *(const float i_lhs, const sVector2& i_rhs)
{
	return i_rhs * i_lhs;
}

constexpr float eae6320::Math::Dot(const sVector2& i_lhs, const sVector2& i_rhs)
{
	return (i_lhs.x * i_rhs.x) + (i_lhs.y * i_rhs.y);
}

constexpr float eae6320::Math::Cross(const sVector2& i_lhs, const sVector2& i_rhs)
{
	return (i_lhs.x * i_rhs.y) - (i_lhs.y * i_rhs.x);
}

// Comparison
//-----------

constexpr bool eae6320::Math::sVector2::operator ==(const sVector2& i_rhs) const
{
	// Use & rather than && to prevent branches (all three comparisons will be evaluated)
	return (x == i_rhs.x) & (y == i_rhs.y);
}

constexpr bool eae6320::Math::sVector2::operator !=(const sVector2& i_rhs) const
{
	// Use | rather than || to prevent branches (all three comparisons will be evaluated)
	return (x != i_rhs.x) | (y != i_rhs.y);
}

// Initialization / Clean Up
//--------------------------

constexpr eae6320::Math::sVector2::sVector2(const float i_x, const float i_y)
	:
	x(i_x), y(i_y)
{

}

#endif	// EAE6320_MATH_SVECTOR_INL
