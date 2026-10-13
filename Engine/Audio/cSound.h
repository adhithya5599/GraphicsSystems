/*
	A sound asset holds the samples of one sound

	It is loaded from a binary file that Tools/SoundBuilder built from a .wav file,
	and it is reference counted like the other assets (cMesh, cEffect, cTexture):
	whoever holds a pointer to it owns a reference,
	and the asset deletes itself when the last reference is released.

	The audio system also holds a reference to every sound that is playing,
	so releasing a sound while it can still be heard is safe:
	the samples stay in memory until the sound finishes (or is stopped)
	instead of being freed while the audio hardware is still reading them.
*/

#ifndef EAE6320_AUDIO_CSOUND_H
#define EAE6320_AUDIO_CSOUND_H

// Includes
//=========

#include "Audio.h"

#include <cstdint>
#include <Engine/Assets/ReferenceCountedAssets.h>
#include <Engine/Results/Results.h>
#include <vector>

// Class Declaration
//==================

namespace eae6320
{
	namespace Audio
	{
		class cSound
		{
			// Interface
			//==========

		public:

			// Playback
			//---------

			// Starts playing the sound (see Audio.h)
			sPlayingSound Play( const sPlaybackSettings& i_settings = {} );

			// Access
			//-------

			uint16_t GetChannelCount() const { return m_channelCount; }
			uint32_t GetSampleRate() const { return m_sampleRate; }
			const std::vector<int16_t>& GetSamples() const { return m_samples; }
			// How long the sound lasts at its original pitch
			float GetDurationInSeconds() const;

			// Initialize / Clean Up
			//----------------------

			// Loads a built (binary) sound file.
			// On success o_sound holds a new reference that the caller must release with DecrementReferenceCount().
			static cResult Load( const char* const i_path, cSound*& o_sound );

			EAE6320_ASSETS_DECLAREDELETEDREFERENCECOUNTEDFUNCTIONS( cSound );

			// Reference Counting
			//-------------------

			EAE6320_ASSETS_DECLAREREFERENCECOUNTINGFUNCTIONS();

			// Data
			//=====

		private:

			// The samples (interleaved if the sound is stereo)
			std::vector<int16_t> m_samples;
			uint32_t m_sampleRate = 0;
			uint16_t m_channelCount = 0;

			EAE6320_ASSETS_DECLAREREFERENCECOUNT();

			// Implementation
			//===============

		private:

			// Initialize / Clean Up
			//----------------------

			// The constructor and destructor are private so that the only way to create an instance is Load()
			// and the only way to destroy one is to release the last reference
			cSound() = default;
			~cSound();
		};
	}
}

#endif	// EAE6320_AUDIO_CSOUND_H
