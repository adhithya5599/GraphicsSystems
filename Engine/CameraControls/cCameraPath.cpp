// Includes
//=========

#include "cCameraPath.h"

#include <Engine/Asserts/Asserts.h>

#include <algorithm>

// Helper Declarations
//====================

namespace
{
	// How fast (and in which direction) the path is moving through keyframe i_index:
	// the slope of the line between the keyframes on either side of it, divided by the time between them.
	// The first and last keyframes have no speed, so the move starts and ends gently.
	template <class tGetPoint>
	eae6320::Math::sVector CalculateVelocity( const std::vector<eae6320::Camera::cCameraPath::sKeyframe>& i_keyframes, const size_t i_index,
		const tGetPoint& i_getPoint );
}

// Interface
//==========

void eae6320::Camera::cCameraPath::SetKeyframes( const std::vector<sKeyframe>& i_keyframes )
{
	EAE6320_ASSERTF( !i_keyframes.empty(), "A camera path needs at least one keyframe" );
	EAE6320_ASSERTF( std::is_sorted( i_keyframes.begin(), i_keyframes.end(),
		[]( const sKeyframe& i_a, const sKeyframe& i_b ) { return i_a.time < i_b.time; } ),
		"A camera path's keyframes must be in order of time" );
	m_keyframes = i_keyframes;
}

float eae6320::Camera::cCameraPath::GetDuration() const
{
	return m_keyframes.empty() ? 0.0f : m_keyframes.back().time;
}

void eae6320::Camera::cCameraPath::CalculateView( const float i_time, Math::sVector& o_position, Math::cQuaternion& o_orientation ) const
{
	if ( m_keyframes.empty() )
	{
		return;
	}

	Math::sVector lookAtPosition;
	if ( ( i_time <= m_keyframes.front().time ) || ( m_keyframes.size() == 1 ) )
	{
		o_position = m_keyframes.front().position;
		lookAtPosition = m_keyframes.front().lookAtPosition;
	}
	else if ( i_time >= m_keyframes.back().time )
	{
		o_position = m_keyframes.back().position;
		lookAtPosition = m_keyframes.back().lookAtPosition;
	}
	else
	{
		// Find the segment (the pair of keyframes) that the time is between
		size_t index = 0;
		while ( m_keyframes[index + 1].time < i_time )
		{
			++index;
		}
		const auto& k1 = m_keyframes[index];
		const auto& k2 = m_keyframes[index + 1];
		const auto duration = k2.time - k1.time;
		const auto t = ( i_time - k1.time ) / duration;
		// A cubic Hermite curve: it passes through both keyframes with the velocity calculated for each of them.
		// Because each keyframe's velocity is the same on both sides of it, the camera never changes speed suddenly
		// (a plain Catmull-Rom curve ignores how long each segment takes, so the speed jumps at keyframes when segments differ in length).
		const auto t2 = t * t;
		const auto t3 = t2 * t;
		const auto h00 = ( 2.0f * t3 ) - ( 3.0f * t2 ) + 1.0f;
		const auto h10 = t3 - ( 2.0f * t2 ) + t;
		const auto h01 = ( -2.0f * t3 ) + ( 3.0f * t2 );
		const auto h11 = t3 - t2;
		const auto Hermite = [&]( const auto& i_getPoint )
		{
			const auto v1 = CalculateVelocity( m_keyframes, index, i_getPoint );
			const auto v2 = CalculateVelocity( m_keyframes, index + 1, i_getPoint );
			return ( i_getPoint( k1 ) * h00 ) + ( v1 * ( h10 * duration ) ) + ( i_getPoint( k2 ) * h01 ) + ( v2 * ( h11 * duration ) );
		};
		o_position = Hermite( []( const sKeyframe& i_keyframe ) { return i_keyframe.position; } );
		lookAtPosition = Hermite( []( const sKeyframe& i_keyframe ) { return i_keyframe.lookAtPosition; } );
	}

	// (LookRotation() builds the orientation of something at o_position facing toward lookAtPosition with +y up)
	o_orientation = Math::cQuaternion::LookRotation( lookAtPosition - o_position, Math::sVector( 0.0f, 1.0f, 0.0f ) );
}

// Helper Definitions
//===================

namespace
{
	template <class tGetPoint>
	eae6320::Math::sVector CalculateVelocity( const std::vector<eae6320::Camera::cCameraPath::sKeyframe>& i_keyframes, const size_t i_index,
		const tGetPoint& i_getPoint )
	{
		if ( ( i_index == 0 ) || ( ( i_index + 1 ) >= i_keyframes.size() ) )
		{
			return eae6320::Math::sVector();
		}
		const auto& before = i_keyframes[i_index - 1];
		const auto& after = i_keyframes[i_index + 1];
		return ( i_getPoint( after ) - i_getPoint( before ) ) / ( after.time - before.time );
	}
}
