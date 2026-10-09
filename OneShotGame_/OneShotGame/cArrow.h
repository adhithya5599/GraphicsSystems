/*
	The arrow that the player launches and then steers toward the target

	The arrow moves itself with its rigid body state (there is no physics simulation pushing it around);
	the physics system is only used to ask whether its colliders touch anything.
*/

#ifndef EAE6320_ONESHOT_CARROW_H
#define EAE6320_ONESHOT_CARROW_H

// Includes
//=========

#include <Engine/GameObject/cGameObject.h>
#include <Engine/Physics/Collision3D.h>

// Forward Declarations
//=====================

namespace eae6320
{
	namespace OneShot
	{
		struct sGameAssets;
	}
}

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cArrow
		{
			// Interface
			//==========

		public:

			// Initialize / Clean Up
			//----------------------

			// Puts the arrow on the bow, ready to be launched
			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets() { m_gameObject.ReleaseAssets(); }

			// Flight
			//-------

			void Launch();
			// Stops the arrow where it is (when it hits something)
			void Stop();
			// Stops the arrow partway along the path it moved in the last update
			// (0 = where it started the update, 1 = where it is now),
			// so that it can stop exactly where a sweep found that it touched something
			// instead of wherever the update happened to end
			void StopPartwayThroughLastUpdate( const float i_fractionOfLastUpdate );
			bool IsFlying() const { return m_isFlying; }

			// Sets how the player wants the arrow to move.
			// The steering inputs are -1, 0, or 1 (left/right and down/up).
			void Steer( const float i_horizontalInput, const float i_verticalInput, const bool i_isBoosting );
			void Update( const float i_elapsedSecondCount_sinceLastUpdate );

			const Math::sVector& GetPosition() const { return m_gameObject.GetRigidBodyState().position; }
			const Math::sVector& GetVelocity() const { return m_gameObject.GetRigidBodyState().velocity; }

			// Collision
			//----------

			// The tip at the start of the last update, and how far it moved during that update
			// (to sweep the tip along its path so that it can't skip over anything thin)
			Physics::sSphere3D GetTipAtStartOfLastUpdate() const;
			Math::sVector GetTipMovementDuringLastUpdate() const;
			Math::sVector GetTipPosition() const;
			// The shaft at the arrow's current position
			Physics::sAabb3D GetShaftCollider() const;

			// Render
			//-------

			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate ) { m_gameObject.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate ); }

			// Data
			//=====

		private:

			GameObject::cGameObject m_gameObject;
			// Where the arrow was at the start of the last update
			Math::sVector m_position_startOfLastUpdate;
			bool m_isFlying = false;

			// Implementation
			//===============

			void RemoveVelocityPastSteeringLimits();
		};
	}
}

#endif	// EAE6320_ONESHOT_CARROW_H
