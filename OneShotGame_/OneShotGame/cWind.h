/*
	The crosswind that pushes the arrow sideways, and the flags that show it

	Wind is a real hazard in outdoor archery: archers watch the flags along the range and aim off to allow for it.
	Here the wind blows across the range (along x) and pushes the arrow sideways while it flies,
	so the player has to steer against it.

	The wind isn't random: it is the sum of two slow waves, so it rises, falls, and changes direction
	in a pattern that a player can read from the flags and plan for
	(the same reason the obstacles move in fixed patterns rather than reacting to the player).
*/

#ifndef EAE6320_ONESHOT_CWIND_H
#define EAE6320_ONESHOT_CWIND_H

// Includes
//=========

#include <Engine/GameObject/cGameObject.h>

#include <array>

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
		class cWind
		{
			// Interface
			//==========

		public:

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();

			void Update( const float i_elapsedSecondCount_sinceLastUpdate );
			// The wind's speed across the range (meters per second; positive blows toward +x)
			float GetSpeed() const { return m_speed; }

			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate );

			// Data
			//=====

		private:

			struct sFlag
			{
				GameObject::cGameObject pole;
				GameObject::cGameObject cloth;
			};
			std::array<sFlag, 3> m_flags;
			float m_elapsedSecondCount = 0.0f;
			float m_speed = 0.0f;

			// Implementation
			//===============

			void UpdateFlags();
		};
	}
}

#endif	// EAE6320_ONESHOT_CWIND_H
