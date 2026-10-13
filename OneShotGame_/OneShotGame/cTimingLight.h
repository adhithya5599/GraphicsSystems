/*
	The timing light at the shooting line, which tells the player when they can shoot

	Real archery competitions control shooting with lights at the shooting line:
	red means stop and green means shoot (and yellow means 30 seconds are left, which this game doesn't use).
	This light is green, with the word "SPACE" under it, while the arrow is on the bow and ready to launch,
	and red the rest of the time (during the opening shot, while the arrow flies, and while the result is shown).
	It is part of the world rather than text over the view (diegetic), like the big screen and the quiver,
	and it is off to the side, so it is there when the player looks for it without getting in the way.

	A lamp is "turned on" by drawing a bright, unlit disc in front of its dark lens.
*/

#ifndef EAE6320_ONESHOT_CTIMINGLIGHT_H
#define EAE6320_ONESHOT_CTIMINGLIGHT_H

// Includes
//=========

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
		class cTimingLight
		{
			// Interface
			//==========

		public:

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();

			// i_canShoot is true while Space will launch the arrow
			void SubmitToBeRendered( const bool i_canShoot );

			// Data
			//=====

		private:

			GameObject::cGameObject m_frame;
			GameObject::cGameObject m_redLamp;
			GameObject::cGameObject m_greenLamp;
			GameObject::cGameObject m_label;
			// Where the light is and which way it faces (every part is modeled in the same local space)
			Math::cMatrix_transformation m_transform_localToWorld;
		};
	}
}

#endif	// EAE6320_ONESHOT_CTIMINGLIGHT_H
