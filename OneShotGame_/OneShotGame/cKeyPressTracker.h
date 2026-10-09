/*
	Detects the moment a key is pressed, rather than whether it is being held

	UserInput::IsKeyPressed() answers "is this key down right now?",
	so using it for a toggle (like pause) flips the toggle on every update while the key is held:
	the original game's pause flickered on and off for as long as P was down.
	A toggle needs "did this key just go down?", which means remembering whether it was down last time.
*/

#ifndef EAE6320_ONESHOT_CKEYPRESSTRACKER_H
#define EAE6320_ONESHOT_CKEYPRESSTRACKER_H

// Includes
//=========

#include <array>
#include <cstdint>

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cKeyPressTracker
		{
			// Interface
			//==========

		public:

			// Returns true only for the first check after the key goes down
			// (holding the key down doesn't return true again until it has been released).
			// Each key should be checked exactly once per frame,
			// because each check is what records the key's state for the next one.
			bool WasKeyJustPressed( const uint_fast8_t i_keyCode );

			// Data
			//=====

		private:

			// Whether each key was down at its last check (indexed by key code)
			std::array<bool, 256> m_wasKeyDown = {};
		};
	}
}

#endif	// EAE6320_ONESHOT_CKEYPRESSTRACKER_H
