/*
	The game's cameras and the switching between them

		* The opening shot sweeps over the whole range so the player can see the obstacles before shooting
		* The third-person camera watches from behind the archer while aiming, and chases the arrow
		* The first-person camera rides on the arrow while it flies
		* The result shot cuts to the crowd after the arrow lands and then turns to the big screen to show the score

	The third- and first-person cameras are tracking cameras whose behavior comes from data files
	(the .lua files in Engine/Content/Cameras, built by CameraBuilder),
	and both are updated all the time (not just the one being looked through)
	so that switching to either one always starts from a camera that is already in the right place.
	Switching blends between views (cCameraTransition) instead of cutting,
	except for the result shot, which cuts on purpose (like a television broadcast cutting to the crowd).
	The opening shot and the result shot are scripted camera moves (cCameraPath) whose keyframes are in GameplaySettings.h.

	Drawing smoothly between simulation updates
		The simulation only updates 15 times a second, but frames are drawn much more often.
		Everything that is drawn is extrapolated to the moment the frame shows (using velocities, or time for the scripted parts),
		so the camera is extrapolated the same way. Otherwise the camera would move in 15-per-second steps
		while the arrow in front of it moved smoothly, which looks like the arrow is jittering.
*/

#ifndef EAE6320_ONESHOT_CSHOTCAMERARIG_H
#define EAE6320_ONESHOT_CSHOTCAMERARIG_H

// Includes
//=========

#include <Engine/CameraControls/cCameraPath.h>
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
				Intro,
				ThirdPerson,
				FirstPerson,
				Result,
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

			void PlayIntro();
			bool IsIntroPlaying() const { return m_view == eView::Intro; }
			// Cuts to the crowd and plays the result shot (which stays on the big screen once it gets there)
			void PlayResultShot();
			// Blends from whatever is being seen now to the given view
			// (switching to the view that is already being used does nothing unless it is forced,
			// which blends from the current view to wherever that view's camera is now)
			void SwitchTo( const eView i_view, const float i_transitionDurationInSeconds, const bool i_shouldForce = false );
			void ToggleArrowView( const float i_transitionDurationInSeconds );
			eView GetView() const { return m_view; }

			// Update
			//-------

			// Tells the tracking cameras what to follow:
			// the third-person camera follows one point and the first-person camera follows another
			void Track( const Math::sVector& i_thirdPersonSubject, const Math::sVector& i_firstPersonSubject );
			// Moves the tracking cameras straight to where they should be instead of letting them ease there
			// (for when what they follow jumps, like the arrow going back onto the bow)
			void SettleTrackingCameras();
			void Update( const float i_elapsedSecondCount_sinceLastUpdate );

			// Render
			//-------

			// Calculates the view at the moment the frame will be shown.
			// i_subjectVelocity is how fast whatever the cameras are following is moving right now
			// (it must be read when the frame is submitted, not saved from the last update:
			// steering changes the arrow's velocity after the update, and a stale velocity makes the camera and arrow drift apart)
			void CalculateView( const float i_elapsedSecondCount_sinceLastSimulationUpdate, const Math::sVector& i_subjectVelocity,
				Math::sVector& o_position, Math::cQuaternion& o_orientation, float& o_verticalFieldOfView ) const;
			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate, const Math::sVector& i_subjectVelocity ) const;

			// Data
			//=====

		private:

			Camera::cTrackingCamera m_thirdPersonCamera;
			Camera::cTrackingCamera m_firstPersonCamera;
			Camera::cCameraPath m_introPath;
			Camera::cCameraPath m_resultPath;
			Camera::cCameraTransition m_transition;

			// The tracking cameras keep pointers to what they track,
			// so these must stay at the same address for as long as the cameras exist
			Math::sVector m_thirdPersonTargetPosition;
			Math::sVector m_firstPersonTargetPosition;
			Math::cQuaternion m_targetOrientation;

			// The view at the last update (where a new transition starts from)
			Math::sVector m_position;
			Math::cQuaternion m_orientation;
			float m_verticalFieldOfView = 1.0f;
			float m_verticalFieldOfView_transitionStart = 1.0f;

			// How far into the scripted camera move (the opening shot or the result shot) the camera is
			float m_pathSecondCount = 0.0f;
			eView m_view = eView::Intro;
			float m_aspectRatio = 1.0f;
			bool m_hasTrackedBefore = false;

			// Implementation
			//===============

			// The view of a single camera (no blending), extrapolated by the given time
			void CalculateUnblendedView( const eView i_view, const float i_extraSeconds, const Math::sVector& i_subjectVelocity,
				Math::sVector& o_position, Math::cQuaternion& o_orientation, float& o_verticalFieldOfView ) const;
			const Camera::cTrackingCamera& GetTrackingCamera( const eView i_view ) const;
		};
	}
}

#endif	// EAE6320_ONESHOT_CSHOTCAMERARIG_H
