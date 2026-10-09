#include "sVector2D.h"
#include <cmath>
#include <limits>
#include <Engine/Asserts/Asserts.h>
#include "Functions.h"

namespace eae6320::Math
{
	// Data
	//=====

	// Constants
	//----------
	const sVector2D sVector2D::Zero(0.f, 0.f);
	const sVector2D sVector2D::Left(-1.f, 0.f);
	const sVector2D sVector2D::Right(1.f, 0.f);
	const sVector2D sVector2D::Up(0.f, 1.f);
	const sVector2D sVector2D::Down(0.f, -1.f);
	const sVector2D sVector2D::One(1.f, 1.f);

	// Interface
	//==========

	// Operators
	//----------

	sVector2D sVector2D::operator/(float i_rhs) const
	{
		if (std::fabs(i_rhs) < std::numeric_limits<float>::epsilon())
		{
			EAE6320_ASSERTF(false, "sVector2D::operator/ : cannot divide by zero");
		}

		return sVector2D(x / i_rhs, y / i_rhs);
	}

	sVector2D& sVector2D::operator/=(float i_rhs)
	{
		if (std::fabs(i_rhs) < std::numeric_limits<float>::epsilon())
		{
			EAE6320_ASSERTF(false, "sVector2D::operator/= : cannot divide by zero");
		}

		x /= i_rhs;
		y /= i_rhs;

		return *this;
	}

	// Comparison
	//-----------

	bool sVector2D::Approximately(const sVector2D& i_lhs, const sVector2D& i_rhs, float i_epsilon)
	{
		return Math::Approximately(i_lhs.x, i_rhs.x, i_epsilon) && Math::Approximately(i_lhs.y, i_rhs.y, i_epsilon);
	}

	// Operations
	//-----------

	float sVector2D::SqrMagnitude() const
	{
		return x * x + y * y;
	}

	float sVector2D::Magnitude() const
	{
		return std::sqrtf(SqrMagnitude());
	}

	sVector2D sVector2D::Normalized() const
	{
		float magnitude = Magnitude();
		if (std::fabs(magnitude) < std::numeric_limits<float>::epsilon())
		{
			EAE6320_ASSERTF(false, "sVector2D::Normalized : cannot divide by zero");
		}

		return *this / magnitude;
	}

	void sVector2D::Normalize()
	{
		float magnitude = Magnitude();
		if (std::fabs(magnitude) < std::numeric_limits<float>::epsilon())
		{
			EAE6320_ASSERTF(false, "sVector2D::Normalize : cannot divide by zero");
		}

		*this /= magnitude;
	}

	// Friends
	//========

	float Dot(const sVector2D& i_lhs, const sVector2D& i_rhs)
	{
		return i_lhs.x * i_rhs.x + i_lhs.y * i_rhs.y;
	}
}
