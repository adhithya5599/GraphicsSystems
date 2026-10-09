// Includes
//=========

#include "sVector2.h"
#include "sVector.h"

#include <cmath>
#include <Engine/Asserts/Asserts.h>

// Static Data
//============

namespace
{
	constexpr auto s_epsilon = 1.0e-9f;
}

// Interface
//==========

// Division
//---------

eae6320::Math::sVector2 eae6320::Math::sVector2::operator /(const float i_rhs) const
{
	EAE6320_ASSERTF(std::abs(i_rhs) > s_epsilon, "Can't divide by zero");
	return sVector2(x / i_rhs, y / i_rhs);
}

eae6320::Math::sVector2& eae6320::Math::sVector2::operator /=(const float i_rhs)
{
	EAE6320_ASSERTF(std::abs(i_rhs) > s_epsilon, "Can't divide by zero");
	x /= i_rhs;
	y /= i_rhs;
	return *this;
}

eae6320::Math::sVector2& eae6320::Math::sVector2::operator =(const sVector& i_other)
{
	x = i_other.x;
	y = i_other.y;
	return *this;
}

// Length / Normalization
//-----------------------

float eae6320::Math::sVector2::GetLength() const
{
	const auto length_squared = (x * x) + (y * y);
	EAE6320_ASSERTF(length_squared >= 0.0f, "Can't take a square root of a negative number");
	return std::sqrt(length_squared);
}

float eae6320::Math::sVector2::GetLengthSquared() const
{
	return (x * x) + (y * y);;
}

float eae6320::Math::sVector2::Normalize()
{
	const auto length = GetLength();
	EAE6320_ASSERTF(length > s_epsilon, "Can't divide by zero");
	operator /=(length);
	return length;
}

float eae6320::Math::sVector2::Distance(const eae6320::Math::sVector2& i_other) const
{
	return (i_other - *this).GetLength();
}

eae6320::Math::sVector2 eae6320::Math::sVector2::GetNormalized() const
{
	const auto length = GetLength();
	EAE6320_ASSERTF(length > s_epsilon, "Can't divide by zero");
	return sVector2(x / length, y / length);
}
