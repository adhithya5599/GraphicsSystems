/*
	A camera path is a scripted camera move ("fly-through") through a list of keyframes

	Each keyframe says where the camera is at a moment in time and what it is looking at.
	Between keyframes the camera follows a smooth curve that passes through every keyframe
	(a cubic Hermite spline whose speed at each keyframe comes from its neighbors, like a Catmull-Rom spline but aware of time),
	so a handful of keyframes are enough for a sweeping move with no sudden changes of direction or speed.
	The camera is still at the first and last keyframes, so the move eases in and out.

	The camera's orientation is calculated from the point it is looking at (which is interpolated the same way)
	rather than by interpolating rotations, which keeps the shot framed on what matters
	and means a keyframe never needs to be authored as a rotation.
*/

#ifndef EAE6320_CAMERA_CCAMERAPATH_H
#define EAE6320_CAMERA_CCAMERAPATH_H

// Includes
//=========

#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector.h>

#include <vector>

// Class Declaration
//==================

namespace eae6320
{
	namespace Camera
	{
		class cCameraPath
		{
			// Interface
			//==========

		public:

			struct sKeyframe
			{
				// When the camera passes through this keyframe (in seconds from the start of the path)
				float time = 0.0f;
				Math::sVector position;
				Math::sVector lookAtPosition;
			};

			// The keyframes must be in order of time
			void SetKeyframes( const std::vector<sKeyframe>& i_keyframes );
			float GetDuration() const;

			// Calculates where the camera is and which way it faces at the given time
			// (times before the start or after the end use the first or last keyframe)
			void CalculateView( const float i_time, Math::sVector& o_position, Math::cQuaternion& o_orientation ) const;

			// Data
			//=====

		private:

			std::vector<sKeyframe> m_keyframes;
		};
	}
}

#endif	// EAE6320_CAMERA_CCAMERAPATH_H
