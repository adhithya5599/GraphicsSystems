/*
	The obstacles between the bow and the target, which move back and forth

	The layout (where each obstacle is and how it moves) is in GameplaySettings.h;
	this class creates the obstacles from that layout, moves them, and checks whether the arrow touches any of them.
*/

#ifndef EAE6320_ONESHOT_COBSTACLECOURSE_H
#define EAE6320_ONESHOT_COBSTACLECOURSE_H

// Includes
//=========

#include "GameplaySettings.h"

#include <Engine/GameObject/cGameObject.h>

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

			// Update
			//-------

			void Update( const float i_elapsedSecondCount_sinceLastUpdate );
			// If the arrow touched an obstacle during its last update,
			// o_timeOfImpact is how far through that update it first touched (0 = the start of the update, 1 = the end)
			bool IsHitBy( const cArrow& i_arrow, float& o_timeOfImpact ) const;

			// Render
			//-------

			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate );

			// Data
			//=====

		private:

			struct sObstacle
			{
				GameObject::cGameObject gameObject;
				Settings::sObstacleLayout layout;
			};
			std::vector<sObstacle> m_obstacles;
			// The obstacles' motion is calculated from how long they have been moving
			float m_elapsedSecondCount = 0.0f;

			// Implementation
			//===============

			void MoveObstacles();
		};
	}
}

#endif	// EAE6320_ONESHOT_COBSTACLECOURSE_H
