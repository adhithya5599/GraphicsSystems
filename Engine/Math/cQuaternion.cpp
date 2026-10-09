// Includes
//=========

#include "cQuaternion.h"

#include "Constants.h"
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

// Normalization
//--------------

void eae6320::Math::cQuaternion::Normalize()
{
	const auto length = std::sqrt( ( m_w * m_w ) + ( m_x * m_x ) + ( m_y * m_y ) + ( m_z * m_z ) );
	EAE6320_ASSERTF( length > s_epsilon, "Can't divide by zero" );
	const auto length_reciprocal = 1.0f / length;
	m_w *= length_reciprocal;
	m_x *= length_reciprocal;
	m_y *= length_reciprocal;
	m_z *= length_reciprocal;
}

eae6320::Math::cQuaternion eae6320::Math::cQuaternion::GetNormalized() const
{
	const auto length = std::sqrt( ( m_w * m_w ) + ( m_x * m_x ) + ( m_y * m_y ) + ( m_z * m_z ) );
	EAE6320_ASSERTF( length > s_epsilon, "Can't divide by zero" );
	const auto length_reciprocal = 1.0f / length;
	return cQuaternion( m_w * length_reciprocal, m_x * length_reciprocal, m_y * length_reciprocal, m_z * length_reciprocal );
}

// Initialize / Clean Up
//----------------------

eae6320::Math::cQuaternion::cQuaternion( const float i_angleInRadians, const sVector i_axisOfRotation_normalized )
{
	const auto theta_half = i_angleInRadians * 0.5f;
	m_w = std::cos( theta_half );
	const auto sin_theta_half = std::sin( theta_half );
	m_x = i_axisOfRotation_normalized.x * sin_theta_half;
	m_y = i_axisOfRotation_normalized.y * sin_theta_half;
	m_z = i_axisOfRotation_normalized.z * sin_theta_half;
}

// (CameraControls new operations)
//================================

// Magnitude
//----------

float eae6320::Math::cQuaternion::Magnitude() const
{
	return std::sqrt(SqrMagnitude());
}

float eae6320::Math::cQuaternion::SqrMagnitude() const
{
	return (m_w * m_w) + (m_x * m_x) + (m_y * m_y) + (m_z * m_z);
}

// Eulers
//-------

eae6320::Math::sVector eae6320::Math::cQuaternion::EulerAngles() const
{
	// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles

	// pitch (m_x-axis rotation)
	float sinr_cosp = 2 * (m_w * m_x + m_y * m_z);
	float cosr_cosp = 1 - 2 * (m_x * m_x + m_y * m_y);
	float pitch = std::atan2(sinr_cosp, cosr_cosp);

	// yaw (m_y-axis rotation)
	float sinp = std::sqrt(1 + 2 * (m_w * m_y - m_x * m_z));
	float cosp = std::sqrt(1 - 2 * (m_w * m_y - m_x * m_z));
	float yaw = 2.f * std::atan2(sinp, cosp) - g_pi / 2.f;

	// roll (m_z-axis rotation)
	float siny_cosp = 2 * (m_w * m_z + m_x * m_y);
	float cosy_cosp = 1 - 2 * (m_y * m_y + m_z * m_z);
	float roll = std::atan2(siny_cosp, cosy_cosp);

	return eae6320::Math::sVector(pitch, yaw, roll);
}

eae6320::Math::cQuaternion eae6320::Math::cQuaternion::FromEulerAngles(const sVector& i_eulerAngles)
{
	// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	float pitch = i_eulerAngles.x;
	float yaw = i_eulerAngles.y;
	float roll = i_eulerAngles.z;

	float cr = std::cos(roll * 0.5f);
	float sr = std::sin(roll * 0.5f);
	float cp = std::cos(pitch * 0.5f);
	float sp = std::sin(pitch * 0.5f);
	float cy = std::cos(yaw * 0.5f);
	float sy = std::sin(yaw * 0.5f);

	float w = cp * cy * cr + sp * sy * sr;
	float x = sp * cy * cr - cp * sy * sr;
	float y = cp * sy * cr + sp * cy * sr;
	float z = cp * cy * sr - sp * sy * cr;

	return cQuaternion(w, x, y, z);
}

// Angle/axis
//-----------

eae6320::Math::sVector eae6320::Math::cQuaternion::Axis() const
{
	return sVector(m_x, m_y, m_z);
}

float eae6320::Math::cQuaternion::AngleBetween(const eae6320::Math::cQuaternion& i_a, const eae6320::Math::cQuaternion& i_b)
{
	float cosHalfAngle = Dot(i_a, i_b);
	cosHalfAngle = std::fmin(std::fabs(cosHalfAngle), 1.f);

	return std::acos(cosHalfAngle) * 2;
}

// Lerp
//-----

eae6320::Math::cQuaternion eae6320::Math::cQuaternion::Slerp(
	const eae6320::Math::cQuaternion& i_from, const eae6320::Math::cQuaternion& i_to, float i_time)
{
	return SlerpUnclamped(i_from, i_to, Clamp(i_time, 0, 1));
}

eae6320::Math::cQuaternion eae6320::Math::cQuaternion::SlerpUnclamped(
	const eae6320::Math::cQuaternion& i_from, const eae6320::Math::cQuaternion& i_to, float i_time)
{
	// https://gist.github.com/HelloKitty/91b7af87aac6796c3da9

	const cQuaternion& a = i_from;
	cQuaternion b(i_to);

	// if either input is zero, return the other.
	if (a.SqrMagnitude() == 0.0f)
	{
		return b.SqrMagnitude() == 0.0f ? cQuaternion() : b;
	}
	else if (b.SqrMagnitude() == 0.0f)
	{
		return a;
	}

	float cosHalfAngle = a.m_w * b.m_w + Dot(a.Axis(), b.Axis());

	if (cosHalfAngle >= 1.0f || cosHalfAngle <= -1.0f)
	{
		// angle = 0.0f, so just return one input.
		return a;
	}
	else if (cosHalfAngle < 0.0f)
	{
		b.m_w = -b.m_w;
		b.m_x = -b.m_x;
		b.m_y = -b.m_y;
		b.m_z = -b.m_z;
		cosHalfAngle = -cosHalfAngle;
	}

	float blendA;
	float blendB;
	if (cosHalfAngle < 0.99f)
	{
		// do proper slerp for big angles
		float halfAngle = std::acos(cosHalfAngle);
		float sinHalfAngle = std::sin(halfAngle);
		float oneOverSinHalfAngle = 1.0f / sinHalfAngle;
		blendA = std::sin(halfAngle * (1.0f - i_time)) * oneOverSinHalfAngle;
		blendB = std::sin(halfAngle * i_time) * oneOverSinHalfAngle;
	}
	else
	{
		// do lerp if angle is really small.
		blendA = 1.0f - i_time;
		blendB = i_time;
	}

	cQuaternion result(
		blendA * a.m_w + blendB * b.m_w,
		blendA * a.m_x + blendB * b.m_x,
		blendA * a.m_y + blendB * b.m_y,
		blendA * a.m_z + blendB * b.m_z
	);

	return result.SqrMagnitude() > 0.0f ? result.GetNormalized() : cQuaternion();
}

// Look
//-----

eae6320::Math::cQuaternion eae6320::Math::cQuaternion::LookRotation(const eae6320::Math::sVector& i_forward, const eae6320::Math::sVector& i_up)
{
	// https://gist.github.com/HelloKitty/91b7af87aac6796c3da9
	// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf

	// reverse the forward vector to convert from left-handed to right-handed coordinate system
	sVector forward = -i_forward.GetNormalized();
	sVector right = Cross(i_up, forward).GetNormalized();
	sVector up = Cross(forward, right);

	float m00 = right.x;
	float m01 = right.y;
	float m02 = right.z;
	float m10 = up.x;
	float m11 = up.y;
	float m12 = up.z;
	float m20 = forward.x;
	float m21 = forward.y;
	float m22 = forward.z;

	float num8 = (m00 + m11) + m22;
	cQuaternion quat;
	if (num8 > 0.f)
	{
		float num = std::sqrtf(num8 + 1.f);
		quat.m_w = num * 0.5f;
		num = 0.5f / num;
		quat.m_x = (m12 - m21) * num;
		quat.m_y = (m20 - m02) * num;
		quat.m_z = (m01 - m10) * num;
		return quat;
	}
	if ((m00 >= m11) && (m00 >= m22))
	{
		float num7 = std::sqrtf(((1.f + m00) - m11) - m22);
		float num4 = 0.5f / num7;
		quat.m_x = 0.5f * num7;
		quat.m_y = (m01 + m10) * num4;
		quat.m_z = (m02 + m20) * num4;
		quat.m_w = (m12 - m21) * num4;
		return quat;
	}
	if (m11 > m22)
	{
		float num6 = std::sqrtf(((1.f + m11) - m00) - m22);
		float num3 = 0.5f / num6;
		quat.m_x = (m10 + m01) * num3;
		quat.m_y = 0.5f * num6;
		quat.m_z = (m21 + m12) * num3;
		quat.m_w = (m20 - m02) * num3;
		return quat;
	}

	float num5 = std::sqrtf(((1.f + m22) - m00) - m11);
	float num2 = 0.5f / num5;
	quat.m_x = (m20 + m02) * num2;
	quat.m_y = (m21 + m12) * num2;
	quat.m_z = 0.5f * num5;
	quat.m_w = (m01 - m10) * num2;
	return quat;
}

// (end CameraControls new operations)