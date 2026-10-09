/*
	A simple camera that moves with its own rigid body state

	This is for games that want to drive a camera directly (e.g. moving it with the keyboard).
	A game that wants a camera to follow something should use Camera::cTrackingCamera (in Engine/CameraControls) instead.
*/

#ifndef EAE6320_GAMEOBJECT_CCAMERA_H
#define EAE6320_GAMEOBJECT_CCAMERA_H

// Includes
//=========

#include <Engine/Math/Functions.h>
#include <Engine/Physics/sRigidBodyState.h>

// Class Declaration
//==================

namespace eae6320
{
	namespace GameObject
	{
		class cCamera
		{
			// Interface
			//==========

		public:

			// Movement
			//---------

			Physics::sRigidBodyState& GetRigidBodyState() { return m_rigidBodyState; }
			const Physics::sRigidBodyState& GetRigidBodyState() const { return m_rigidBodyState; }

			// Projection
			//-----------

			void SetVerticalFieldOfView( const float i_verticalFieldOfView_inRadians ) { m_verticalFieldOfView_inRadians = i_verticalFieldOfView_inRadians; }
			// Objects closer than the near plane or further away than the far plane aren't drawn.
			// Keeping the ratio far / near small gives the depth buffer more precision.
			void SetClippingPlanes( const float i_nearPlaneDistance, const float i_farPlaneDistance );

			// Render
			//-------

			// Submits the camera for the frame that is currently being submitted
			// (extrapolated by the time since the last simulation update, like a game object)
			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate, const float i_aspectRatio ) const;

			// Data
			//=====

		private:

			Physics::sRigidBodyState m_rigidBodyState;

			float m_verticalFieldOfView_inRadians = Math::ConvertDegreesToRadians( 45.0f );
			float m_nearPlaneDistance = 0.1f;
			float m_farPlaneDistance = 100.0f;
		};
	}
}

#endif	// EAE6320_GAMEOBJECT_CCAMERA_H
