/*
	The targets at the end of the range, and how a hit is scored

	The player's target is in the middle of the range; the targets on either side are only scenery
	(they are outside of the area the arrow can be steered in).

	A hit is scored like a real World Archery 122 cm face:
	ten rings, each 6.1 cm wide, worth 10 points in the middle down to 1 point at the edge.
	An arrow that hits the straw boss outside of the rings scores nothing.

	Arrows that hit the target stay stuck in it until the round is over, like in a real end,
	so the player can see their group of arrows.
*/

#ifndef EAE6320_ONESHOT_CTARGET_H
#define EAE6320_ONESHOT_CTARGET_H

// Includes
//=========

#include <Engine/GameObject/cGameObject.h>
#include <Engine/Math/sVector.h>

#include <array>
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
		class cTarget
		{
			// Interface
			//==========

		public:

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();

			// If the arrow's tip crossed the front of the player's target boss during its last update this returns true,
			// with how far through that update it hit (0 = the start of the update, 1 = the end)
			// and the score (1 to 10, or 0 for hitting the boss outside of the rings)
			bool IsHitBy( const cArrow& i_arrow, float& o_timeOfImpact, unsigned int& o_score ) const;

			// Leaves an arrow stuck in the target with its nock at the given position (it points straight down the range, like the flying arrow)
			void StickArrow( const Math::sVector& i_nockPosition );
			// Pulls every stuck arrow out (when a new round starts)
			void RemoveStuckArrows() { m_stuckArrowNockPositions.clear(); }

			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate );

			// Data
			//=====

		private:

			// The player's target is first
			std::array<GameObject::cGameObject, 5> m_targets;
			// One arrow game object is drawn once for each stuck arrow
			GameObject::cGameObject m_stuckArrow;
			std::vector<Math::sVector> m_stuckArrowNockPositions;
		};
	}
}

#endif	// EAE6320_ONESHOT_CTARGET_H
