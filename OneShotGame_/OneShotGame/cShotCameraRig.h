/*
	The game's two cameras and the switching between them

		* The third-person camera watches from the side while aiming and when the shot is over
		* The first-person camera rides on the arrow while it flies

	Both cameras are tracking cameras whose behavior comes from data files
	(the .lua files in Engine/Content/Cameras, built by CameraBuilder),
	and both are updated all the time (not just the one being looked through)
	so that switching to either one always starts from a camera that is already in the right place.
	Switching blends between them (cCameraTransition) instead of cutting.
*/

#ifndef EAE6320_ONESHOT_CSHOTCAMERARIG_H
#define EAE6320_ONESHOT_CSHOTCAMERARIG_H

// Includes
//=========

#include <Engine/CameraControls/cCameraTransition.h>
#include <Engine/CameraControls/cTrackingCamera.h>
#include <Engine/Results/Results.h>

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cShotCameraRig
		{
			// Interface
			//==========

		public:

			enum class eView
			{
				ThirdPerson,
				FirstPerson,
			};

			// Initialize
			//-----------

			cResult Load( const float i_aspectRatio );

			cShotCameraRig() = default;
			// The tracking cameras keep pointers to this object's members,
			// so moving it would leave them pointing at the old (destroyed) object
			cShotCameraRig( const cShotCameraRig& ) = delete;
			cShotCameraRig( cShotCameraRig&& ) = delete;
			cShotCameraRig& operator =( const cShotCameraRig& ) = delete;
			cShotCameraRig& operator =( cShotCameraRig&& ) = delete;

			// Views
			//------

			// Switches to the given view, blending from whatever is being seen now
			void SwitchTo( const eView i_view );
			void ToggleView() { SwitchTo( ( m_view == eView::ThirdPerson ) ? eView::FirstPerson : eView::ThirdPerson ); }
			eView GetView() const { return m_view; }

			// Update
			//-------

			// Tells the cameras what to follow:
			// the third-person camera follows one point and the first-person camera follows another,
			// and the velocity is how fast whatever is being followed is moving
			// (it is used to extrapolate the view between simulation updates, like the game objects are)
			void Track( const Math::sVector& i_thirdPersonSubject, const Math::sVector& i_firstPersonSubject, const Math::sVector& i_subjectVelocity );
			void Update( const float i_elapsedSecondCount_sinceLastUpdate );

			// Render
			//-------

			// Where the camera will be rendered from (e.g. so that the sky can be centered on it)
			Math::sVector PredictFuturePosition( const float i_elapsedSecondCount_sinceLastSimulationUpdate ) const;
			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate ) const;

			// Data
			//=====

		private:

			Camera::cTrackingCamera m_thirdPersonCamera;
			Camera::cTrackingCamera m_firstPersonCamera;
			Camera::cCameraTransition m_transition;

			// The tracking cameras keep pointers to what they track,
			// so these must stay at the same address for as long as the cameras exist
			Math::sVector m_thirdPersonTargetPosition;
			Math::sVector m_firstPersonTargetPosition;
			Math::cQuaternion m_targetOrientation;
			Math::sVector m_subjectVelocity;

			// The view that is actually rendered (the blend of the two cameras while switching)
			Math::sVector m_position;
			Math::cQuaternion m_orientation;
			float m_verticalFieldOfView = 1.0f;
			float m_verticalFieldOfView_transitionStart = 1.0f;

			eView m_view = eView::ThirdPerson;
			float m_aspectRatio = 1.0f;
			bool m_hasTrackedBefore = false;

			// Implementation
			//===============

			const Camera::cTrackingCamera& GetCamera( const eView i_view ) const;
		};
	}
}

#endif	// EAE6320_ONESHOT_CSHOTCAMERARIG_H
