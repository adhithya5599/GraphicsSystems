// Includes
//=========

#include "cKeyPressTracker.h"

#include <Engine/UserInput/UserInput.h>

// Interface
//==========

bool eae6320::OneShot::cKeyPressTracker::WasKeyJustPressed( const uint_fast8_t i_keyCode )
{
	const auto isKeyDown = UserInput::IsKeyPressed( i_keyCode );
	auto& wasKeyDown = m_wasKeyDown[i_keyCode];
	const auto wasJustPressed = isKeyDown && !wasKeyDown;
	wasKeyDown = isKeyDown;
	return wasJustPressed;
}
