#ifndef EAE6320_MATH_SVECTOR_INL
#define EAE6320_MATH_SVECTOR_INL

// Includes
//=========

#include "sVector.h"
#include "Functions.h"

// Interface
//==========

// Addition
//---------

constexpr eae6320::Math::sVector eae6320::Math::sVector::operator +( const sVector& i_rhs ) const
{
	return sVector( x + i_rhs.x, y + i_rhs.y, z + i_rhs.z );
}

constexpr eae6320::Math::sVector& eae6320::Math::sVector::operator +=( const sVector& i_rhs )
{
	x += i_rhs.x;
	y += i_rhs.y;
	z += i_rhs.z;
	return *this;
}

constexpr eae6320::Math::sVector eae6320::Math::sVector::operator +( const float i_rhs ) const
{
	return sVector( x + i_rhs, y + i_rhs, z + i_rhs );
}

constexpr eae6320::Math::sVector& eae6320::Math::sVector::operator +=( const float i_rhs )
{
	x += i_rhs;
	y += i_rhs;
	z += i_rhs;
	return *this;
}

constexpr eae6320::Math::sVector eae6320::Math::operator +( const float i_lhs, const sVector& i_rhs )
{
	return i_rhs + i_lhs;
}

// Subtraction / Negation
//-----------------------

constexpr eae6320::Math::sVector eae6320::Math::sVector::operator -( const sVector& i_rhs ) const
{
	return sVector( x - i_rhs.x, y - i_rhs.y, z - i_rhs.z );
}

constexpr eae6320::Math::sVector& eae6320::Math::sVector::operator -=( const sVector& i_rhs )
{
	x -= i_rhs.x;
	y -= i_rhs.y;
	z -= i_rhs.z;
	return *this;
}

constexpr eae6320::Math::sVector eae6320::Math::sVector::operator -() const
{
	return sVector( -x, -y, -z );
}

constexpr eae6320::Math::sVector eae6320::Math::sVector::operator -( const float i_rhs ) const
{
	return sVector( x - i_rhs, y - i_rhs, z - i_rhs );
}

constexpr eae6320::Math::sVector& eae6320::Math::sVector::operator -=( const float i_rhs )
{
	x -= i_rhs;
	y -= i_rhs;
	z -= i_rhs;
	return *this;
}

constexpr eae6320::Math::sVector eae6320::Math::operator -( const float i_lhs, const sVector& i_rhs )
{
	return sVector( i_lhs - i_rhs.x, i_lhs - i_rhs.y, i_lhs - i_rhs.z );
}

// Products
//---------

constexpr eae6320::Math::sVector eae6320::Math::sVector::operator *( const float i_rhs ) const
{
	return sVector( x * i_rhs, y * i_rhs, z * i_rhs );
}

constexpr eae6320::Math::sVector& eae6320::Math::sVector::operator *=( const float i_rhs )
{
	x *= i_rhs;
	y *= i_rhs;
	z *= i_rhs;
	return *this;
}

constexpr eae6320::Math::sVector eae6320::Math::operator *( const float i_lhs, const sVector& i_rhs )
{
	return i_rhs * i_lhs;
}

constexpr float eae6320::Math::Dot( const sVector& i_lhs, const sVector& i_rhs )
{
	return ( i_lhs.x * i_rhs.x ) + ( i_lhs.y * i_rhs.y ) + ( i_lhs.z * i_rhs.z );
}

constexpr eae6320::Math::sVector eae6320::Math::Cross( const sVector& i_lhs, const sVector& i_rhs )
{
	return sVector(
		( i_lhs.y * i_rhs.z ) - ( i_lhs.z * i_rhs.y ),
		( i_lhs.z * i_rhs.x ) - ( i_lhs.x * i_rhs.z ),
		( i_lhs.x * i_rhs.y ) - ( i_lhs.y * i_rhs.x )
	);
}

// Comparison
//-----------

constexpr bool eae6320::Math::sVector::operator ==( const sVector& i_rhs ) const
{
	// Use & rather than && to prevent branches (all three comparisons will be evaluated)
	return ( x == i_rhs.x ) & ( y == i_rhs.y ) & ( z == i_rhs.z );
}

constexpr bool eae6320::Math::sVector::operator !=( const sVector& i_rhs ) const
{
	// Use | rather than || to prevent branches (all three comparisons will be evaluated)
	return ( x != i_rhs.x ) | ( y != i_rhs.y ) | ( z != i_rhs.z );
}

// (CameraControls new operations)
//================================

// Lerp
//-----

constexpr bool eae6320::Math::sVector::Approximately(const sVector& i_lhs, const sVector& i_rhs, float i_epsilon)
{
	return Math::Approximately(i_lhs.x, i_rhs.x, i_epsilon)
		&& Math::Approximately(i_lhs.y, i_rhs.y, i_epsilon)
		&& Math::Approximately(i_lhs.z, i_rhs.z, i_epsilon);
}

constexpr eae6320::Math::sVector eae6320::Math::sVector::Lerp(const sVector& i_from, const sVector& i_to, float i_time)
{
	return LerpUnclamped(i_from, i_to, Math::Clamp(i_time, 0, 1));
}

constexpr eae6320::Math::sVector eae6320::Math::sVector::LerpUnclamped(const sVector& i_from, const sVector& i_to, float i_time)
{
	return sVector(
		Math::LerpUnclamped(i_from.x, i_to.x, i_time),
		Math::LerpUnclamped(i_from.y, i_to.y, i_time),
		Math::LerpUnclamped(i_from.z, i_to.z, i_time)
	);
}

// Clamp
//------

constexpr eae6320::Math::sVector eae6320::Math::sVector::Clamp(const sVector& i_value, const sVector& i_min, const sVector& i_max)
{
	return sVector(
		Math::Clamp(i_value.x, i_min.x, i_max.x),
		Math::Clamp(i_value.y, i_min.y, i_max.y),
		Math::Clamp(i_value.z, i_min.z, i_max.z)
	);
}

// Damping
//--------

constexpr eae6320::Math::sVector eae6320::Math::sVector::Damp(
	const sVector& i_from, const sVector& i_to, float i_halflife, float i_deltaSeconds)
{
	return Damp(i_from, i_to, sVector(i_halflife, i_halflife, i_halflife), i_deltaSeconds);
}

inline constexpr eae6320::Math::sVector eae6320::Math::sVector::Damp(
	const sVector& i_from, const sVector& i_to, const sVector& i_halflife, float i_deltaSeconds)
{
	return sVector(
		Math::Damp(i_from.x, i_to.x, i_halflife.x, i_deltaSeconds),
		Math::Damp(i_from.y, i_to.y, i_halflife.y, i_deltaSeconds),
		Math::Damp(i_from.z, i_to.z, i_halflife.z, i_deltaSeconds)
	);
}

constexpr eae6320::Math::sVector eae6320::Math::sVector::SmoothDamp(
	const sVector& i_from, const sVector& i_to, float i_smoothTime, float i_deltaSeconds, sVector& io_velocity)
{
	return SmoothDamp(i_from, i_to, sVector(i_smoothTime, i_smoothTime, i_smoothTime), i_deltaSeconds, io_velocity);
}

inline constexpr eae6320::Math::sVector eae6320::Math::sVector::SmoothDamp(
	const sVector& i_from, const sVector& i_to, const sVector& i_smoothTime, float i_deltaSeconds, sVector& io_velocity)
{
	return sVector(
		Math::SmoothDamp(i_from.x, i_to.x, i_smoothTime.x, i_deltaSeconds, io_velocity.x),
		Math::SmoothDamp(i_from.y, i_to.y, i_smoothTime.y, i_deltaSeconds, io_velocity.y),
		Math::SmoothDamp(i_from.z, i_to.z, i_smoothTime.z, i_deltaSeconds, io_velocity.z)
	);
}

// (end CameraControls new operations)

// Initialization / Clean Up
//--------------------------

constexpr eae6320::Math::sVector::sVector( const float i_x, const float i_y, const float i_z )
	:
	x( i_x ), y( i_y ), z( i_z )
{

}

#endif	// EAE6320_MATH_SVECTOR_INL
