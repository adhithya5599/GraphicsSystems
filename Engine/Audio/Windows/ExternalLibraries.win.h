/*
	This file lists external prebuilt libraries that this library depends on
*/

#pragma once

// External Libraries
//===================

// XAudio2 is part of the Windows SDK and of Windows itself (Windows 10 and later have XAudio2 2.9 built in),
// so it works the same way for both the Direct3D and the OpenGL builds
#pragma comment( lib, "xaudio2.lib" )
#pragma comment( lib, "Ole32.lib" )
