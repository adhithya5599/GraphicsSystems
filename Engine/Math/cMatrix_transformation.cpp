// Includes
//=========

#include "cMatrix_transformation.h"

#include "cQuaternion.h"
#include "sVector.h"

#include <cmath>

// Interface
//==========

// Access
//-------

const eae6320::Math::sVector& eae6320::Math::cMatrix_transformation::GetRightDirection() const
{
	return *reinterpret_cast<const sVector*>( &m_00 );
}

const eae6320::Math::sVector& eae6320::Math::cMatrix_transformation::GetUpDirection() const
{
	return *reinterpret_cast<const sVector*>( &m_01 );
}

const eae6320::Math::sVector& eae6320::Math::cMatrix_transformation::GetBackDirection() const
{
	return *reinterpret_cast<const sVector*>( &m_02 );
}

const eae6320::Math::sVector& eae6320::Math::cMatrix_transformation::GetTranslation() const
{
	return *reinterpret_cast<const sVector*>( &m_03 );
}

void eae6320::Math::cMatrix_transformation::SetLocation(const eae6320::Math::sVector& i_location)
{
	m_03 = i_location.x;
	m_13 = i_location.y;
	m_23 = i_location.z;
}

void eae6320::Math::cMatrix_transformation::SetRotation(const eae6320::Math::cQuaternion& i_rotation)
{
	const float w = i_rotation.m_w;
	const float x = i_rotation.m_x;
	const float y = i_rotation.m_y;
	const float z = i_rotation.m_z;

	const float xx = x * x;
	const float yy = y * y;
	const float zz = z * z;
	const float xy = x * y;
	const float xz = x * z;
	const float yz = y * z;
	const float wx = w * x;
	const float wy = w * y;
	const float wz = w * z;

	m_00 = 1.0f - 2.0f * (yy + zz);
	m_10 = 2.0f * (xy + wz);
	m_20 = 2.0f * (xz - wy);
	m_30 = 0.0f;

	m_01 = 2.0f * (xy - wz);
	m_11 = 1.0f - 2.0f * (xx + zz);
	m_21 = 2.0f * (yz + wx);
	m_31 = 0.0f;

	m_02 = 2.0f * (xz + wy);
	m_12 = 2.0f * (yz - wx);
	m_22 = 1.0f - 2.0f * (xx + yy);
	m_32 = 0.0f;
}

eae6320::Math::sVector eae6320::Math::cMatrix_transformation::GetLocation() const
{
	return eae6320::Math::sVector(m_03, m_13, m_23);
}

eae6320::Math::cQuaternion eae6320::Math::cMatrix_transformation::GetRotation() const
{
	float w, x, y, z;

	const float trace = m_00 + m_11 + m_22;

	if (trace > 0.0f)
	{
		const float s = std::sqrt(trace + 1.0f) * 2.0f; // s = 4 * w
		w = 0.25f * s;
		x = (m_21 - m_12) / s;
		y = (m_02 - m_20) / s;
		z = (m_10 - m_01) / s;
	}
	else if ((m_00 > m_11) && (m_00 > m_22))
	{
		const float s = std::sqrt(1.0f + m_00 - m_11 - m_22) * 2.0f; // s = 4 * x
		w = (m_21 - m_12) / s;
		x = 0.25f * s;
		y = (m_01 + m_10) / s;
		z = (m_02 + m_20) / s;
	}
	else if (m_11 > m_22)
	{
		const float s = std::sqrt(1.0f + m_11 - m_00 - m_22) * 2.0f; // s = 4 * y
		w = (m_02 - m_20) / s;
		x = (m_01 + m_10) / s;
		y = 0.25f * s;
		z = (m_12 + m_21) / s;
	}
	else
	{
		const float s = std::sqrt(1.0f + m_22 - m_00 - m_11) * 2.0f; // s = 4 * z
		w = (m_10 - m_01) / s;
		x = (m_02 + m_20) / s;
		y = (m_12 + m_21) / s;
		z = 0.25f * s;
	}

	return cQuaternion(w, x, y, z);
}

void eae6320::Math::cMatrix_transformation::Translate(const eae6320::Math::sVector& i_vector)
{
	m_03 += i_vector.x;
	m_13 += i_vector.y;
	m_23 += i_vector.z;
}

// Camera
//-------

eae6320::Math::cMatrix_transformation eae6320::Math::cMatrix_transformation::CreateCameraToProjectedTransform_perspective(
	const float i_verticalFieldOfView_inRadians,
	const float i_aspectRatio,
	const float i_z_nearPlane, const float i_z_farPlane )
{
	const auto yScale = 1.0f / std::tan( i_verticalFieldOfView_inRadians * 0.5f );
	const auto xScale = yScale / i_aspectRatio;
#if defined( EAE6320_PLATFORM_D3D )
	const auto zDistanceScale = i_z_farPlane / ( i_z_nearPlane - i_z_farPlane );
	return cMatrix_transformation(
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, zDistanceScale, -1.0f,
		0.0f, 0.0f, i_z_nearPlane * zDistanceScale, 0.0f );
#elif defined( EAE6320_PLATFORM_GL )
	const auto zDistanceScale = 1.0f / ( i_z_nearPlane - i_z_farPlane );
	return cMatrix_transformation(
		xScale, 0.0f, 0.0f, 0.0f,
		0.0f, yScale, 0.0f, 0.0f,
		0.0f, 0.0f, ( i_z_nearPlane + i_z_farPlane ) * zDistanceScale, -1.0f,
		0.0f, 0.0f, ( 2.0f * i_z_nearPlane * i_z_farPlane ) * zDistanceScale, 0.0f );
#endif
}

// Initialize / Clean Up
//----------------------

eae6320::Math::cMatrix_transformation::cMatrix_transformation( const cQuaternion& i_rotation, const sVector& i_translation )
	:
	m_30( 0.0f ), m_31( 0.0f ), m_32( 0.0f ),
	m_03( i_translation.x ), m_13( i_translation.y ), m_23( i_translation.z ),
	m_33( 1.0f )
{
	const auto _2x = i_rotation.m_x + i_rotation.m_x;
	const auto _2y = i_rotation.m_y + i_rotation.m_y;
	const auto _2z = i_rotation.m_z + i_rotation.m_z;
	const auto _2xx = i_rotation.m_x * _2x;
	const auto _2xy = _2x * i_rotation.m_y;
	const auto _2xz = _2x * i_rotation.m_z;
	const auto _2xw = _2x * i_rotation.m_w;
	const auto _2yy = _2y * i_rotation.m_y;
	const auto _2yz = _2y * i_rotation.m_z;
	const auto _2yw = _2y * i_rotation.m_w;
	const auto _2zz = _2z * i_rotation.m_z;
	const auto _2zw = _2z * i_rotation.m_w;

	m_00 = 1.0f - _2yy - _2zz;
	m_01 = _2xy - _2zw;
	m_02 = _2xz + _2yw;

	m_10 = _2xy + _2zw;
	m_11 = 1.0f - _2xx - _2zz;
	m_12 = _2yz - _2xw;

	m_20 = _2xz - _2yw;
	m_21 = _2yz + _2xw;
	m_22 = 1.0f - _2xx - _2yy;
}
