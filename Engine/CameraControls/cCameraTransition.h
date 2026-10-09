/*
	A camera transition blends smoothly from one camera's view to another's
	instead of cutting instantly.

	When a game switches cameras (e.g. from a third-person view to a first-person view)
	it starts a transition from wherever the old camera was.
	Every update, the game then asks the transition for a view
	somewhere between that starting view and wherever the new camera currently is.
	Because the destination is read every update rather than remembered,
	the blend still ends exactly on the new camera even if that camera keeps moving while blending.
*/

#ifndef EAE6320_CAMERA_CCAMERATRANSITION_H
#define EAE6320_CAMERA_CCAMERATRANSITION_H

// Includes
//=========

#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector.h>

// Class Declaration
//==================

namespace eae6320
{
	namespace Camera
	{
		class cCameraTransition
		{
			// Interface
			//==========

		public:

			// Starts blending away from the given view.
			// A duration of 0 (or less) is an instant cut.
			void Start( const Math::sVector& i_fromPosition, const Math::cQuaternion& i_fromOrientation, const float i_durationInSeconds );
			// Advances the blend
			void Update( const float i_elapsedSecondCount );
			bool IsBlending() const { return m_elapsedSecondCount < m_durationInSeconds; }
			// How far from the start view to the destination view the blend is (0 = start, 1 = destination).
			// This can be used to blend other camera settings (like the field of view) the same way.
			float GetBlendWeight() const;

			// Calculates the view that should be used right now:
			// the start view while the blend begins, moving towards the destination view, and exactly the destination view once finished
			void CalculateView( const Math::sVector& i_toPosition, const Math::cQuaternion& i_toOrientation,
				Math::sVector& o_position, Math::cQuaternion& o_orientation ) const;

			// Data
			//=====

		private:

			Math::sVector m_fromPosition;
			Math::cQuaternion m_fromOrientation;
			float m_durationInSeconds = 0.0f;
			float m_elapsedSecondCount = 0.0f;
		};
	}
}

#endif	// EAE6320_CAMERA_CCAMERATRANSITION_H
