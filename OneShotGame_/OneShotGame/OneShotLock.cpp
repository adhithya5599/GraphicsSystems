// Includes
//=========

#include "OneShotLock.h"

#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>

#include <fstream>
#include <string>

// Helper Declarations
//====================

namespace
{
	std::string GetLockFilePath();
}

// Interface
//==========

bool eae6320::OneShot::Lock::HasShotBeenTaken()
{
#ifdef _DEBUG
	return false;
#else
	return Platform::DoesFileExist( GetLockFilePath().c_str() );
#endif
}

void eae6320::OneShot::Lock::RecordThatShotWasTaken()
{
#ifdef _DEBUG
	Logging::OutputMessage( "The shot was taken (this Debug build doesn't record it, so the game can be played again)" );
#else
	const auto path = GetLockFilePath();
	std::ofstream lockFile( path );
	if ( lockFile.is_open() )
	{
		lockFile << "You only get one shot";
	}
	else
	{
		Logging::OutputError( "The one-shot lock file \"%s\" couldn't be written", path.c_str() );
	}
#endif
}

// Helper Definitions
//===================

namespace
{
	std::string GetLockFilePath()
	{
		return eae6320::Platform::GetAppDataFolderPath() + "/OneShotGame.lock";
	}
}
