// Includes
//=========

#include "sVector.h"
#include "cQuaternion.h"

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

eae6320::Math::sVector eae6320::Math::sVector::operator /( const float i_rhs ) const
{
	EAE6320_ASSERTF( std::abs( i_rhs ) > s_epsilon, "Can't divide by zero" );
	return sVector( x / i_rhs, y / i_rhs, z / i_rhs );
}

eae6320::Math::sVector& eae6320::Math::sVector::operator /=( const float i_rhs )
{
	EAE6320_ASSERTF( std::abs( i_rhs ) > s_epsilon, "Can't divide by zero" );
	x /= i_rhs;
	y /= i_rhs;
	z /= i_rhs;
	return *this;
}

// Length / Normalization
//-----------------------

float eae6320::Math::sVector::GetLength() const
{
	const auto length_squared = ( x * x ) + ( y * y ) + ( z * z );
	EAE6320_ASSERTF( length_squared >= 0.0f, "Can't take a square root of a negative number" );
	return std::sqrt( length_squared );
}

float eae6320::Math::sVector::Normalize()
{
	const auto length = GetLength();
	EAE6320_ASSERTF( length > s_epsilon, "Can't divide by zero" );
	operator /=( length );
	return length;
}

eae6320::Math::sVector eae6320::Math::sVector::GetNormalized() const
{
	const auto length = GetLength();
	EAE6320_ASSERTF( length > s_epsilon, "Can't divide by zero" );
	return sVector( x / length, y / length, z / length );
}

// (CameraControls new operations)
//================================

// Projection
//-----------

eae6320::Math::sVector eae6320::Math::sVector::Project(const sVector& i_toProject, const sVector& i_projectedOnto)
{
	float magnitude = Dot(i_toProject, i_projectedOnto) * Dot(i_projectedOnto, i_projectedOnto);
	return magnitude * i_projectedOnto.GetNormalized();
}

eae6320::Math::sVector eae6320::Math::sVector::ProjectOntoPlane(const sVector& i_toProject, const sVector& i_planeNormal)
{
	return i_toProject - Project(i_toProject, i_planeNormal);
}

// Screen space
//-------------

eae6320::Math::sVector2D eae6320::Math::WorldSpaceToScreenSpace(
	const eae6320::Math::sVector& i_worldPosition,
	const eae6320::Math::sVector& i_cameraPosition,
	const eae6320::Math::cQuaternion& i_cameraOrientation,
	const eae6320::Math::sVector2D& i_fieldOfViewRadians)
{
	sVector positionDelta = i_worldPosition - i_cameraPosition;
	sVector positionDeltaLocal = i_cameraOrientation.GetInverse() * positionDelta;

	float tanAngleX = positionDeltaLocal.x / positionDeltaLocal.z;
	float tanAngleY = positionDeltaLocal.y / positionDeltaLocal.z;

	float screenSpaceX = -tanAngleX / std::tan(i_fieldOfViewRadians.x / 2);
	float screenSpaceY = -tanAngleY / std::tan(i_fieldOfViewRadians.y / 2);

	return sVector2D(screenSpaceX, screenSpaceY);
}

eae6320::Math::sVector eae6320::Math::ScreenSpaceToWorldSpace(
	const eae6320::Math::sVector2D& i_screenPosition,
	const eae6320::Math::sVector& i_cameraPosition,
	const eae6320::Math::cQuaternion& i_cameraOrientation,
	const eae6320::Math::sVector2D& i_fieldOfViewRadians)
{
	float tanAngleX = i_screenPosition.x * std::tan(i_fieldOfViewRadians.x / 2);
	float tanAngleY = i_screenPosition.y * std::tan(i_fieldOfViewRadians.y / 2);

	sVector localPosition = sVector(tanAngleX, tanAngleY, -1.f).GetNormalized();

	return i_cameraPosition + (i_cameraOrientation * localPosition);
}

// (end CameraControls new operations)
