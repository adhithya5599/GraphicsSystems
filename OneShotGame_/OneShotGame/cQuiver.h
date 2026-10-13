/*
	The ground quiver beside the archer, which holds the arrows that are left in the round

	It shows how many arrows are left without any numbers on the screen:
	an arrow disappears from the quiver every time one is put on the bow, and the quiver is full again when a new round starts.
*/

#ifndef EAE6320_ONESHOT_CQUIVER_H
#define EAE6320_ONESHOT_CQUIVER_H

// Includes
//=========

#include "GameplaySettings.h"

#include <array>
#include <Engine/GameObject/cGameObject.h>
#include <Engine/Math/cMatrix_transformation.h>

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
		class cQuiver
		{
			// Interface
			//==========

		public:

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();

			// Draws the quiver with this many arrows in it
			void SubmitToBeRendered( const unsigned int i_arrowCount );

			// Data
			//=====

		private:

			GameObject::cGameObject m_quiver;
			// One arrow game object is drawn once for each arrow in the quiver, each time in a different place
			GameObject::cGameObject m_arrow;
			std::array<Math::cMatrix_transformation, Settings::arrowsPerRound> m_arrowTransforms;
		};
	}
}

#endif	// EAE6320_ONESHOT_CQUIVER_H
