// Includes
//=========

#include "cSoundBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <Engine/Audio/sSoundFile.h>
#include <Engine/Platform/Platform.h>
#include <limits>
#include <string>
#include <Tools/AssetBuildLibrary/Functions.h>

// Static Data
//============

namespace
{
	// The format codes that a .wav file's "fmt " chunk uses
	constexpr uint16_t s_formatCode_integer = 0x0001;	// (WAVE_FORMAT_PCM)
	constexpr uint16_t s_formatCode_floatingPoint = 0x0003;	// (WAVE_FORMAT_IEEE_FLOAT)
	// Files with more than two channels or more than 16 bits use this "extensible" code,
	// and the real format code is the first two bytes of a GUID later in the chunk
	constexpr uint16_t s_formatCode_extensible = 0xFFFE;	// (WAVE_FORMAT_EXTENSIBLE)
}

// Helper Declarations
//====================

namespace
{
	// .wav files are little-endian (as are Windows PCs),
	// but reading byte by byte doesn't depend on that and doesn't need the data to be aligned
	uint16_t ReadUint16( const uint8_t* const i_data );
	uint32_t ReadUint32( const uint8_t* const i_data );
	bool AreCharactersEqual( const uint8_t* const i_data, const char* const i_fourCharacters );
}

// Inherited Implementation
//=========================

// Build
//------

eae6320::cResult eae6320::Assets::cSoundBuilder::Build( const std::vector<std::string>& )
{
	auto result = Results::Success;

	// Load the source file
	Platform::sDataFromFile dataFromFile;
	{
		std::string errorMessage;
		if ( !( result = Platform::LoadBinaryFile( m_path_source, dataFromFile, &errorMessage ) ) )
		{
			OutputErrorMessageWithFileInfo( m_path_source, errorMessage.c_str() );
			return result;
		}
	}

	// Find the samples and their format
	sSourceFormat format;
	const uint8_t* sampleData = nullptr;
	size_t sampleDataSize = 0;
	if ( !( result = ReadWaveFile( reinterpret_cast<const uint8_t*>( dataFromFile.data ), dataFromFile.size,
		format, sampleData, sampleDataSize ) ) )
	{
		return result;
	}

	// Convert the samples
	std::vector<int16_t> samples;
	{
		const auto frameCount = sampleDataSize / format.bytesPerFrame;
		if ( ( sampleDataSize % format.bytesPerFrame ) != 0 )
		{
			// Some audio editors write a "data" chunk that ends partway through a frame;
			// the incomplete frame can't be played, so it is dropped
			OutputWarningMessageWithFileInfo( m_path_source, "The samples end partway through a frame; the last %u bytes are ignored",
				static_cast<unsigned int>( sampleDataSize % format.bytesPerFrame ) );
		}
		if ( frameCount == 0 )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( m_path_source, "The sound has no samples" );
			return result;
		}
		ConvertSamples( format, sampleData, frameCount * format.channelCount, samples );
	}

	// Write the built file: the header followed by the samples
	{
		Audio::sFileHeader header;
		header.channelCount = format.channelCount;
		header.sampleRate = format.sampleRate;
		const auto sampleDataSize_built = samples.size() * sizeof( samples[0] );
		if ( sampleDataSize_built > std::numeric_limits<uint32_t>::max() )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( m_path_source, "The sound is too long" );
			return result;
		}
		header.sampleDataSize = static_cast<uint32_t>( sampleDataSize_built );

		std::vector<uint8_t> fileData( sizeof( header ) + sampleDataSize_built );
		memcpy( fileData.data(), &header, sizeof( header ) );
		memcpy( fileData.data() + sizeof( header ), samples.data(), sampleDataSize_built );

		std::string errorMessage;
		if ( !( result = Platform::WriteBinaryFile( m_path_target, fileData.data(), fileData.size(), &errorMessage ) ) )
		{
			OutputErrorMessageWithFileInfo( m_path_target, errorMessage.c_str() );
			return result;
		}
	}

	return result;
}

// Implementation
//===============

eae6320::cResult eae6320::Assets::cSoundBuilder::ReadWaveFile( const uint8_t* const i_fileData, const size_t i_fileSize,
	sSourceFormat& o_format, const uint8_t*& o_sampleData, size_t& o_sampleDataSize ) const
{
	// The file starts with "RIFF", the size of the rest of the file, and "WAVE"
	constexpr size_t riffHeaderSize = 12;
	if ( ( i_fileSize < riffHeaderSize ) || !AreCharactersEqual( i_fileData, "RIFF" ) || !AreCharactersEqual( i_fileData + 8, "WAVE" ) )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "This isn't a .wav file (it doesn't start with \"RIFF\" and \"WAVE\")" );
		return Results::InvalidFile;
	}

	// The rest of the file is a list of chunks.
	// Each chunk is a 4-character name, a 4-byte size, and then that many bytes of data
	// (plus one byte of padding if the size is odd, so that the next chunk starts at an even offset).
	bool wasFormatFound = false;
	o_sampleData = nullptr;
	o_sampleDataSize = 0;
	constexpr size_t chunkHeaderSize = 8;
	size_t offset = riffHeaderSize;
	while ( ( offset + chunkHeaderSize ) <= i_fileSize )
	{
		const auto* const chunkName = i_fileData + offset;
		const auto chunkSize = ReadUint32( i_fileData + offset + 4 );
		const auto* const chunkData = i_fileData + offset + chunkHeaderSize;
		const auto bytesLeftInFile = i_fileSize - ( offset + chunkHeaderSize );
		if ( AreCharactersEqual( chunkName, "fmt " ) )
		{
			if ( chunkSize > bytesLeftInFile )
			{
				OutputErrorMessageWithFileInfo( m_path_source, "The file is truncated (the \"fmt \" chunk is incomplete)" );
				return Results::InvalidFile;
			}
			if ( const auto result = ReadFormatChunk( chunkData, chunkSize, o_format ); !result )
			{
				return result;
			}
			wasFormatFound = true;
		}
		else if ( AreCharactersEqual( chunkName, "data" ) )
		{
			if ( !wasFormatFound )
			{
				OutputErrorMessageWithFileInfo( m_path_source, "The \"data\" chunk comes before the \"fmt \" chunk, so the samples' format is unknown" );
				return Results::InvalidFile;
			}
			o_sampleData = chunkData;
			o_sampleDataSize = chunkSize;
			if ( chunkSize > bytesLeftInFile )
			{
				// Some programs that write .wav files while recording never go back to fill in the size;
				// the samples that are actually there can still be used
				OutputWarningMessageWithFileInfo( m_path_source, "The \"data\" chunk says it is %u bytes but only %u bytes are in the file;"
					" only those are used", static_cast<unsigned int>( chunkSize ), static_cast<unsigned int>( bytesLeftInFile ) );
				o_sampleDataSize = bytesLeftInFile;
			}
			// Nothing after the samples matters
			break;
		}
		// Move to the next chunk (skipping any chunk that isn't needed).
		// A chunk that claims to be bigger than the rest of the file means the file is corrupted,
		// and there is no way to find the next chunk.
		if ( chunkSize >= bytesLeftInFile )
		{
			break;
		}
		offset += chunkHeaderSize + static_cast<size_t>( chunkSize ) + ( chunkSize % 2 );
	}

	if ( !wasFormatFound )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The file has no \"fmt \" chunk" );
		return Results::InvalidFile;
	}
	if ( !o_sampleData )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The file has no \"data\" chunk" );
		return Results::InvalidFile;
	}
	return Results::Success;
}

eae6320::cResult eae6320::Assets::cSoundBuilder::ReadFormatChunk( const uint8_t* const i_chunkData, const uint32_t i_chunkSize,
	sSourceFormat& o_format ) const
{
	constexpr uint32_t minimumChunkSize = 16;
	if ( i_chunkSize < minimumChunkSize )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The \"fmt \" chunk is too small (%u bytes)", static_cast<unsigned int>( i_chunkSize ) );
		return Results::InvalidFile;
	}
	auto formatCode = ReadUint16( i_chunkData + 0 );
	o_format.channelCount = ReadUint16( i_chunkData + 2 );
	o_format.sampleRate = ReadUint32( i_chunkData + 4 );
	// (Bytes 8 to 11 are the number of bytes per second, which can be calculated from the other values)
	o_format.bytesPerFrame = ReadUint16( i_chunkData + 12 );
	o_format.bitsPerSample = ReadUint16( i_chunkData + 14 );
	if ( formatCode == s_formatCode_extensible )
	{
		// The extensible format adds 24 bytes, and the real format code is at the start of the GUID at the end of them
		constexpr uint32_t extensibleChunkSize = 40;
		if ( i_chunkSize < extensibleChunkSize )
		{
			OutputErrorMessageWithFileInfo( m_path_source, "The \"fmt \" chunk is too small (%u bytes) for the extensible format",
				static_cast<unsigned int>( i_chunkSize ) );
			return Results::InvalidFile;
		}
		formatCode = ReadUint16( i_chunkData + 24 );
	}

	// Check that the engine can play it
	if ( formatCode == s_formatCode_integer )
	{
		o_format.sampleType = sSourceFormat::eSampleType::Integer;
		if ( ( o_format.bitsPerSample != 8 ) && ( o_format.bitsPerSample != 16 ) && ( o_format.bitsPerSample != 24 ) && ( o_format.bitsPerSample != 32 ) )
		{
			OutputErrorMessageWithFileInfo( m_path_source, "%u-bit samples aren't supported (only 8, 16, 24, and 32 bits are)",
				static_cast<unsigned int>( o_format.bitsPerSample ) );
			return Results::InvalidFile;
		}
	}
	else if ( formatCode == s_formatCode_floatingPoint )
	{
		o_format.sampleType = sSourceFormat::eSampleType::FloatingPoint;
		if ( o_format.bitsPerSample != 32 )
		{
			OutputErrorMessageWithFileInfo( m_path_source, "%u-bit floating-point samples aren't supported (only 32-bit ones are)",
				static_cast<unsigned int>( o_format.bitsPerSample ) );
			return Results::InvalidFile;
		}
	}
	else
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The samples are compressed (format code 0x%04x);"
			" save the sound as uncompressed PCM", static_cast<unsigned int>( formatCode ) );
		return Results::InvalidFile;
	}
	if ( ( o_format.channelCount == 0 ) || ( o_format.channelCount > Audio::MaxChannelCount ) )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The sound has %u channels but only mono and stereo sounds are supported",
			static_cast<unsigned int>( o_format.channelCount ) );
		return Results::InvalidFile;
	}
	if ( ( o_format.sampleRate < Audio::MinSampleRate ) || ( o_format.sampleRate > Audio::MaxSampleRate ) )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The sample rate (%u Hz) must be between %u and %u Hz",
			static_cast<unsigned int>( o_format.sampleRate ), static_cast<unsigned int>( Audio::MinSampleRate ),
			static_cast<unsigned int>( Audio::MaxSampleRate ) );
		return Results::InvalidFile;
	}
	if ( o_format.bytesPerFrame != ( o_format.channelCount * ( o_format.bitsPerSample / 8 ) ) )
	{
		OutputErrorMessageWithFileInfo( m_path_source, "The \"fmt \" chunk is inconsistent: %u channels of %u-bit samples should be %u bytes per frame, not %u",
			static_cast<unsigned int>( o_format.channelCount ), static_cast<unsigned int>( o_format.bitsPerSample ),
			static_cast<unsigned int>( o_format.channelCount * ( o_format.bitsPerSample / 8 ) ), static_cast<unsigned int>( o_format.bytesPerFrame ) );
		return Results::InvalidFile;
	}

	return Results::Success;
}

void eae6320::Assets::cSoundBuilder::ConvertSamples( const sSourceFormat& i_format, const uint8_t* const i_sampleData, const size_t i_sampleCount,
	std::vector<int16_t>& o_samples )
{
	o_samples.resize( i_sampleCount );
	const auto bytesPerSample = static_cast<size_t>( i_format.bitsPerSample / 8 );
	for ( size_t i = 0; i < i_sampleCount; ++i )
	{
		const auto* const sample = i_sampleData + ( i * bytesPerSample );
		int16_t convertedSample = 0;
		if ( i_format.sampleType == sSourceFormat::eSampleType::FloatingPoint )
		{
			// Floating-point samples go from -1 to 1 (but can go past that, so they are clamped)
			float value;
			memcpy( &value, sample, sizeof( value ) );
			if ( std::isnan( value ) )
			{
				value = 0.0f;
			}
			value = std::clamp( value, -1.0f, 1.0f );
			convertedSample = static_cast<int16_t>( std::lround( value * 32767.0f ) );
		}
		else
		{
			switch ( bytesPerSample )
			{
			case 1:
				// 8-bit samples are unsigned, with silence at 128
				convertedSample = static_cast<int16_t>( ( static_cast<int>( sample[0] ) - 128 ) * 256 );
				break;
			case 2:
				convertedSample = static_cast<int16_t>( ReadUint16( sample ) );
				break;
			case 3:
				// Keep the most significant 16 bits
				convertedSample = static_cast<int16_t>( ReadUint16( sample + 1 ) );
				break;
			case 4:
				convertedSample = static_cast<int16_t>( ReadUint16( sample + 2 ) );
				break;
			}
		}
		o_samples[i] = convertedSample;
	}
}

// Helper Definitions
//===================

namespace
{
	uint16_t ReadUint16( const uint8_t* const i_data )
	{
		return static_cast<uint16_t>( i_data[0] | ( i_data[1] << 8 ) );
	}

	uint32_t ReadUint32( const uint8_t* const i_data )
	{
		return static_cast<uint32_t>( i_data[0] ) | ( static_cast<uint32_t>( i_data[1] ) << 8 )
			| ( static_cast<uint32_t>( i_data[2] ) << 16 ) | ( static_cast<uint32_t>( i_data[3] ) << 24 );
	}

	bool AreCharactersEqual( const uint8_t* const i_data, const char* const i_fourCharacters )
	{
		return memcmp( i_data, i_fourCharacters, 4 ) == 0;
	}
}
