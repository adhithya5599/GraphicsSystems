/*
	The layout of a built sound file

	Tools/SoundBuilder reads a .wav file, checks that it is something the engine can play,
	and writes this header followed by the raw samples.
	The engine then only has to check the header and copy the samples
	(it never has to understand the .wav format, which has many optional parts and variations).

	This file is shared by the builder and the engine so that they can't disagree about the layout.
*/

#ifndef EAE6320_AUDIO_SSOUNDFILE_H
#define EAE6320_AUDIO_SSOUNDFILE_H

// Includes
//=========

#include <cstdint>

// Constants
//==========

namespace eae6320
{
	namespace Audio
	{
		constexpr uint32_t FileMagic = 0x444E5553;	// The bytes "SUND"
		// Increment this whenever sFileHeader changes
		constexpr uint32_t FileVersion = 1;

		// Every sound is stored as signed 16-bit samples (the most common format, and plenty for a game)
		constexpr uint16_t BitsPerSample = 16;
		// Mono sounds and stereo sounds are both supported
		constexpr uint16_t MaxChannelCount = 2;
		// The range of sample rates that the builder accepts
		constexpr uint32_t MinSampleRate = 8000;
		constexpr uint32_t MaxSampleRate = 192000;
	}
}

// Data Types
//===========

namespace eae6320
{
	namespace Audio
	{
		struct sFileHeader
		{
			uint32_t magic = FileMagic;
			uint32_t version = FileVersion;
			uint16_t channelCount = 1;
			uint16_t bitsPerSample = BitsPerSample;
			uint32_t sampleRate = 44100;
			// The size (in bytes) of the samples that follow the header.
			// Stereo samples are interleaved (left, right, left, right, ...).
			uint32_t sampleDataSize = 0;
		};
		static_assert( sizeof( sFileHeader ) == 20, "The sound file header must have no padding so that its layout is the same everywhere" );
	}
}

#endif	// EAE6320_AUDIO_SSOUNDFILE_H
