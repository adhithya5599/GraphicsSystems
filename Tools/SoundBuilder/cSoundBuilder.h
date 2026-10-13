/*
	This class builds sound files

	It reads a .wav file, checks that it is a sound the engine can play,
	converts its samples to 16 bits if necessary,
	and writes the binary file that Engine/Audio loads at run time (see Engine/Audio/sSoundFile.h).

	A .wav file is a "RIFF" file: a list of chunks, each with a 4-letter name and a size.
	Only two chunks matter here:
		* "fmt " says how the samples are stored (how many channels, how many samples per second, how many bits per sample)
		* "data" holds the samples
	Every other chunk (e.g. "LIST", which audio editors use for the artist's name and other metadata) is skipped.

	Doing this at build time rather than at run time means:
		* A .wav file that the engine can't play is an error in Visual Studio's Error List with the file name,
			instead of a game that is silently missing a sound
		* The engine doesn't need to understand the .wav format; loading a sound is checking a header and copying the samples
		* Sounds exported from any audio editor (8, 16, 24, or 32 bits, or 32-bit floating point) can be used without converting them by hand
*/

#ifndef EAE6320_CSOUNDBUILDER_H
#define EAE6320_CSOUNDBUILDER_H

// Includes
//=========

#include <Tools/AssetBuildLibrary/iBuilder.h>

#include <cstdint>
#include <vector>

// Class Declaration
//==================

namespace eae6320
{
	namespace Assets
	{
		class cSoundBuilder final : public iBuilder
		{
			// Inherited Implementation
			//=========================

		private:

			// Build
			//------

			cResult Build( const std::vector<std::string>& i_arguments ) final;

			// Implementation
			//===============

		private:

			// How the samples in the source file are stored
			struct sSourceFormat
			{
				enum class eSampleType
				{
					Integer,
					FloatingPoint,
				};
				eSampleType sampleType = eSampleType::Integer;
				uint16_t channelCount = 0;
				uint32_t sampleRate = 0;
				uint16_t bitsPerSample = 0;
				uint16_t bytesPerFrame = 0;	// (one sample for every channel)
			};

			// Finds and validates the "fmt " and "data" chunks
			cResult ReadWaveFile( const uint8_t* const i_fileData, const size_t i_fileSize,
				sSourceFormat& o_format, const uint8_t*& o_sampleData, size_t& o_sampleDataSize ) const;
			cResult ReadFormatChunk( const uint8_t* const i_chunkData, const uint32_t i_chunkSize, sSourceFormat& o_format ) const;
			// Converts every sample to a signed 16-bit integer
			static void ConvertSamples( const sSourceFormat& i_format, const uint8_t* const i_sampleData, const size_t i_sampleCount,
				std::vector<int16_t>& o_samples );
		};
	}
}

#endif	// EAE6320_CSOUNDBUILDER_H
