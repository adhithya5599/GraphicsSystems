/*
	The obstacles between the archer and the target

	Every obstacle is something that could really be put on an archery range:
	a fixed frame (scaffolding or sandbags) with a wooden part that moves in a way the frame explains:
		* Rail slider:	a plank hanging from a trolley that slides left and right along a rail between two posts
		* Bunker:		two boards hidden behind sandbags that spring up when the arrow gets close
		* Pulley:		a board hanging on ropes from a gantry that is raised and lowered
		* Sled:			a plank on a sled that slides back and forth along tracks on the ground

	The obstacles don't "think": they move in fixed, repeating patterns (or, for the bunker, react to one simple trigger)
	so that a player can watch them during the opening shot, learn their rhythm, and plan a route.
	Each moving part is described by a motion (a kind of movement plus its timing) in GameplaySettings.h.
*/

#ifndef EAE6320_ONESHOT_COBSTACLECOURSE_H
#define EAE6320_ONESHOT_COBSTACLECOURSE_H

// Includes
//=========

#include <Engine/GameObject/cGameObject.h>
#include <Engine/Physics/Collision3D.h>

#include <vector>

// Forward Declarations
//=====================

namespace eae6320
{
	namespace OneShot
	{
		class cArrow;
		struct sGameAssets;
	}
}

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cObstacleCourse
		{
			// Interface
			//==========

		public:

			// Initialize / Clean Up
			//----------------------

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();
			// Puts anything that was triggered by the last shot back the way it started (the bunker's boards go back down)
			void Reset();

			// Update
			//-------

			// The arrow's tip position is needed for the obstacles that react to the arrow
			// (they only react while it is flying, not to an arrow resting on the bow or stuck in something)
			void Update( const float i_elapsedSecondCount_sinceLastUpdate, const Math::sVector& i_arrowTipPosition, const bool i_isArrowFlying );
			// If the arrow touched an obstacle during its last update,
			// o_timeOfImpact is how far through that update it first touched (0 = the start of the update, 1 = the end)
			bool IsHitBy( const cArrow& i_arrow, float& o_timeOfImpact ) const;

			// Render
			//-------

			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate );

			// Data
			//=====

		private:

			// How a moving part moves
			enum class eMotion
			{
				// Swings back and forth along x: offset = amplitude * sin( 2 * pi * ( time / period + phase ) )
				SwingX,
				// The same, along y
				SwingY,
				// Waits hidden until the arrow is within the trigger distance and then rises once
				PopUp,
			};
			struct sMovingPart
			{
				GameObject::cGameObject gameObject;
				// The collider in the moving mesh's local space
				Physics::sAabb3D collider_local;
				// The position that the motion is relative to
				Math::sVector basePosition;
				eMotion motion = eMotion::SwingX;
				float amplitude = 0.0f;
				float periodInSeconds = 1.0f;
				float phase = 0.0f;
				// For pop-ups
				float triggerDistance = 0.0f;
				float riseHeight = 0.0f;
				float riseDurationInSeconds = 1.0f;
				float riseProgress = 0.0f;
				bool isRising = false;
			};
			std::vector<sMovingPart> m_movingParts;
			// The frames, which don't move
			std::vector<GameObject::cGameObject> m_frames;
			// The parts of the frames that the arrow can hit, in world space
			std::vector<Physics::sAabb3D> m_frameColliders;
			// The pulley board's ropes are stretched to reach from the pulleys to wherever the board is
			std::vector<GameObject::cGameObject> m_ropes;
			size_t m_pulleyBoardIndex = 0;

			float m_elapsedSecondCount = 0.0f;

			// Implementation
			//===============

			void MoveParts( const float i_elapsedSecondCount_sinceLastUpdate, const Math::sVector& i_arrowTipPosition, const bool i_isArrowFlying );
			void StretchRopes( const float i_elapsedSecondCount_sinceLastSimulationUpdate );
		};
	}
}

#endif	// EAE6320_ONESHOT_COBSTACLECOURSE_H
