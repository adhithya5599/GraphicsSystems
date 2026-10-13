/*
	The big screen at the end of the range, which shows the round's score

	This is the game's "diegetic" user interface: instead of numbers drawn over the view,
	the score is on a stadium video wall in the world, the way a real competition shows it.

	The engine can't draw text, so the screen is built from meshes:
		* The cabinet (lit like everything else)
		* The display: the black background, the labels that never change, and a bar under each arrow's slot.
			It is unlit, because a screen gives off its own light.
		* One small quad mesh for each character that a number can use ("0" to "9", "M", and "-").
			Each one shows its character from the scoreboard texture,
			and drawing a number means drawing the right character meshes side by side, scaled to the right size.
			One game object per character is enough: it can be drawn as many times as needed in a frame,
			each time with a different transform.
*/

#ifndef EAE6320_ONESHOT_CSCOREBOARD_H
#define EAE6320_ONESHOT_CSCOREBOARD_H

// Includes
//=========

#include "sGameAssets.h"

#include <array>
#include <cstddef>
#include <Engine/GameObject/cGameObject.h>
#include <Engine/Math/cMatrix_transformation.h>

// Forward Declarations
//=====================

namespace eae6320
{
	namespace OneShot
	{
		class cRound;
	}
}

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cScoreboard
		{
			// Interface
			//==========

		public:

			void Initialize( const sGameAssets& i_assets );
			void ReleaseAssets();

			// While this is on, the newest score flashes (to draw the eye to it)
			void SetIsNewestScoreFlashing( const bool i_isFlashing ) { m_isNewestScoreFlashing = i_isFlashing; }

			// The screen reads the round every frame, so it always shows the current score.
			// The flashing uses real (system) time so that it keeps the same rhythm in slow motion.
			void SubmitToBeRendered( const cRound& i_round, const float i_elapsedSecondCount_systemTime );

			// Data
			//=====

		private:

			GameObject::cGameObject m_frame;
			GameObject::cGameObject m_display;
			// One game object per character mesh (see GetGlyphIndex())
			std::array<GameObject::cGameObject, sGameAssets::scoreGlyphCount> m_glyphs;
			// Where the scoreboard is and which way it faces
			Math::cMatrix_transformation m_transform_localToWorld;
			bool m_isNewestScoreFlashing = false;

			// Implementation
			//===============

			// Each of these draws a line of characters on the screen.
			// Positions are in meters from the middle of the screen (x to the right and y up),
			// i_bottom is the bottom of the characters' dots, and i_height is the height of the dots.
			void SubmitText( const char* const i_text, const float i_left, const float i_bottom, const float i_height );
			void SubmitTextRightAligned( const char* const i_text, const float i_right, const float i_bottom, const float i_height );
			void SubmitTextCentered( const char* const i_text, const float i_center, const float i_bottom, const float i_height );

			// Returns which character mesh shows a character, or -1 for a space (which draws nothing)
			static int GetGlyphIndex( const char i_character );
		};
	}
}

#endif	// EAE6320_ONESHOT_CSCOREBOARD_H
