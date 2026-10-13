/*
	The crowd in the grandstand, which jumps up and down when it cheers

	The crowd is split into a few groups (each group is a mesh of people spread all over the stands).
	When the crowd cheers, every group hops, each a little out of step with the others,
	so the stands look busy rather than like one block moving up and down.
	The hops get smaller until the crowd settles back into its seats.
*/

#ifndef EAE6320_ONESHOT_CCROWD_H
#define EAE6320_ONESHOT_CCROWD_H

// Includes
//=========

#include "sGameAssets.h"

#include <array>
#include <Engine/GameObject/cGameObject.h>

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cCrowd
		{
			// Interface
			//==========

		public:

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();

			// Starts a cheer: i_excitement is how excited the crowd is (0 to 1), which is how high they jump
			void Cheer( const float i_excitement );
			// Sits everyone back down straight away
			void Settle();

			void Update( const float i_elapsedSecondCount_sinceLastUpdate );
			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate );

			// Data
			//=====

		private:

			std::array<GameObject::cGameObject, sGameAssets::crowdGroupCount> m_groups;
			float m_cheerSecondCount = 0.0f;
			float m_excitement = 0.0f;

			// Implementation
			//===============

			// How high a group is off its seats at a time in the cheer
			float CalculateHopHeight( const size_t i_groupIndex, const float i_cheerSecondCount ) const;
		};
	}
}

#endif	// EAE6320_ONESHOT_CCROWD_H
