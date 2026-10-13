// Includes
//=========

#include "cCameraTransition.h"

#include <algorithm>

// Interface
//==========

void eae6320::Camera::cCameraTransition::Start( const Math::sVector& i_fromPosition, const Math::cQuaternion& i_fromOrientation,
	const float i_durationInSeconds )
{
	m_fromPosition = i_fromPosition;
	m_fromOrientation = i_fromOrientation;
	m_durationInSeconds = std::max( i_durationInSeconds, 0.0f );
	m_elapsedSecondCount = 0.0f;
}

void eae6320::Camera::cCameraTransition::Update( const float i_elapsedSecondCount )
{
	m_elapsedSecondCount = std::min( m_elapsedSecondCount + i_elapsedSecondCount, m_durationInSeconds );
}

float eae6320::Camera::cCameraTransition::GetBlendWeight( const float i_extraSeconds ) const
{
	const auto elapsedSecondCount = m_elapsedSecondCount + i_extraSeconds;
	if ( elapsedSecondCount >= m_durationInSeconds )
	{
		return 1.0f;
	}
	// How far through the blend this is in time, from 0 to 1
	const auto progress = std::max( elapsedSecondCount, 0.0f ) / m_durationInSeconds;
	// "Smoothstep" eases in and out: the camera starts moving slowly, speeds up, and then slows down as it arrives.
	// A straight (linear) blend would start and stop moving instantly, which reads as a jolt.
	return progress * progress * ( 3.0f - ( 2.0f * progress ) );
}

void eae6320::Camera::cCameraTransition::CalculateView( const Math::sVector& i_toPosition, const Math::cQuaternion& i_toOrientation,
	Math::sVector& o_position, Math::cQuaternion& o_orientation, const float i_extraSeconds ) const
{
	const auto weight = GetBlendWeight( i_extraSeconds );
	if ( weight >= 1.0f )
	{
		o_position = i_toPosition;
		o_orientation = i_toOrientation;
		return;
	}

	o_position = m_fromPosition + ( ( i_toPosition - m_fromPosition ) * weight );
	// Orientations are blended with a spherical interpolation
	// so that the camera turns at a steady rate along the shortest arc
	// (blending the four quaternion numbers directly would speed up and slow down in the middle of the turn)
	o_orientation = Math::cQuaternion::Slerp( m_fromOrientation, i_toOrientation, weight );
}
