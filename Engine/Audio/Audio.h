/*
	The audio system plays sounds

	The application initializes it (after Graphics) and cleans it up,
	and a game loads cSound assets and plays them:
		auto playingSound = sound->Play( { 0.8f } );
	Playing a sound is "fire and forget": the sound plays to the end on its own.
	The returned sPlayingSound only needs to be kept if the game wants to change or stop the sound later
	(e.g. to stop a looping sound).

	Missing audio is never fatal:
	if there is no audio device (e.g. a remote desktop with no speakers) the audio system logs that it is disabled,
	every sound still loads, and playing a sound does nothing.
	The same happens if the audio device is lost while the game is running (e.g. headphones are unplugged).
	A game without sound is still playable, so it shouldn't refuse to start.

	Every function can be called from any thread.
*/

#ifndef EAE6320_AUDIO_H
#define EAE6320_AUDIO_H

// Includes
//=========

#include <Engine/Results/Results.h>

#include <cstdint>

// Forward Declarations
//=====================

namespace eae6320
{
	namespace Audio
	{
		class cSound;
	}
}

// Data Types
//===========

namespace eae6320
{
	namespace Audio
	{
		// How to play a sound
		struct sPlaybackSettings
		{
			// 1 is the volume the sound was recorded at and 0 is silent
			float volume = 1.0f;
			// 1 is the original pitch, 2 is an octave higher, and 0.5 is an octave lower
			// (changing the pitch also changes the speed, like playing a record faster or slower).
			// Varying it slightly each time a sound plays keeps a repeated sound from sounding robotic.
			float pitch = 1.0f;
			// A looping sound plays until it is stopped
			bool shouldLoop = false;
		};

		// A handle to a sound that is playing.
		// It stays safe to use after the sound finishes:
		// the audio system reuses voices, and each handle remembers which use of a voice it belongs to,
		// so an old handle can never stop or change a different sound that is now playing on the same voice.
		struct sPlayingSound
		{
			uint16_t voiceIndex = 0;
			uint16_t generation = 0;	// 0 means that the handle doesn't refer to anything

			bool IsValid() const { return generation != 0; }
		};
	}
}

// Interface
//==========

namespace eae6320
{
	namespace Audio
	{
		// Playback
		//---------

		// Starts playing a sound (cSound::Play() calls this).
		// The returned handle is invalid if the sound couldn't be played (e.g. if audio is disabled).
		sPlayingSound Play( cSound& i_sound, const sPlaybackSettings& i_settings = {} );
		// Stops a sound immediately
		// (the handle is reset so that it is obvious that it doesn't refer to anything anymore)
		void Stop( sPlayingSound& io_playingSound );
		// Changes the volume of a sound that is playing
		// (the change is smoothed over a few milliseconds so that it doesn't click)
		void SetVolume( const sPlayingSound& i_playingSound, const float i_volume );
		// Returns true if the sound hasn't finished yet
		bool IsPlaying( const sPlayingSound& i_playingSound );

		// Returns false if there is no audio device (or if the audio device was lost while the game was running)
		bool IsEnabled();

		// Pausing stops every sound where it is, and resuming carries on from the same place.
		// A sound that is played while audio is paused waits to start until audio is resumed.
		void SetIsPaused( const bool i_isPaused );

		// Initialize / Clean Up
		//----------------------

		// Not being able to play audio isn't an error (see the comment at the top of this file):
		// if audio can't be initialized this logs why and leaves audio disabled
		cResult Initialize();
		// This stops every sound and releases the references that the voices hold
		cResult CleanUp();
	}
}

#endif	// EAE6320_AUDIO_H
