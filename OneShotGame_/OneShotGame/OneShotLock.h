/*
	"You only get one shot": remembers on this computer that the shot has been taken

	A small file in the user's app data folder records that the arrow was launched.
	If the file exists when the game starts, the game tells the player and closes.

	Debug builds ignore the file completely (they never read or write it)
	so that the game can be tested over and over;
	Release builds are the real, one-shot game.
	To play a Release build again, delete OneShotGame.lock from the app data folder.
*/

#ifndef EAE6320_ONESHOT_ONESHOTLOCK_H
#define EAE6320_ONESHOT_ONESHOTLOCK_H

// Interface
//==========

namespace eae6320
{
	namespace OneShot
	{
		namespace Lock
		{
			bool HasShotBeenTaken();
			void RecordThatShotWasTaken();
		}
	}
}

#endif	// EAE6320_ONESHOT_ONESHOTLOCK_H
