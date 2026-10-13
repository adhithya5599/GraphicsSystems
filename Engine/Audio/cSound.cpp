// Includes
//=========

#include "cSound.h"

#include "sSoundFile.h"

#include <cstring>
#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <exception>
#include <new>
#include <string>

// Interface
//==========

// Playback
//---------

eae6320::Audio::sPlayingSound eae6320::Audio::cSound::Play( const sPlaybackSettings& i_settings )
{
	return Audio::Play( *this, i_settings );
}

// Access
//-------

float eae6320::Audio::cSound::GetDurationInSeconds() const
{
	return ( ( m_sampleRate > 0 ) && ( m_channelCount > 0 ) )
		? ( static_cast<float>( m_samples.size() / m_channelCount ) / static_cast<float>( m_sampleRate ) ) : 0.0f;
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::Audio::cSound::Load( const char* const i_path, cSound*& o_sound )
{
	auto result = Results::Success;

	cSound* newSound = nullptr;
	// This follows the same pattern as the other assets' Load() functions:
	// the output is only assigned if everything succeeds,
	// and if anything fails the partially-created asset is released
	cScopeGuard scopeGuard( [&o_sound, &result, &newSound]
		{
			if ( result )
			{
				EAE6320_ASSERT( newSound != nullptr );
				o_sound = newSound;
			}
			else
			{
				if ( newSound )
				{
					newSound->DecrementReferenceCount();
					newSound = nullptr;
				}
				o_sound = nullptr;
			}
		} );

	// Load the binary data
	Platform::sDataFromFile dataFromFile;
	{
		std::string errorMessage;
		if ( !( result = Platform::LoadBinaryFile( i_path, dataFromFile, &errorMessage ) ) )
		{
			Logging::OutputError( "Failed to load the sound file \"%s\": %s", i_path, errorMessage.c_str() );
			return result;
		}
	}

	// Validate the header.
	// The builder already validated the .wav file,
	// so the only things that can be wrong here are a file that isn't a sound file,
	// one that was built by a different version of the builder, or one that has been truncated.
	sFileHeader header;
	{
		if ( dataFromFile.size < sizeof( header ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The sound file \"%s\" is too small (%u bytes) to be a sound file",
				i_path, static_cast<unsigned int>( dataFromFile.size ) );
			return result;
		}
		memcpy( &header, dataFromFile.data, sizeof( header ) );
		if ( header.magic != FileMagic )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The file \"%s\" isn't a built sound file", i_path );
			return result;
		}
		if ( header.version != FileVersion )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The sound file \"%s\" was built with a different version of SoundBuilder"
				" (file version %u, expected version %u); rebuild the game's assets", i_path,
				static_cast<unsigned int>( header.version ), static_cast<unsigned int>( FileVersion ) );
			return result;
		}
		if ( ( header.bitsPerSample != BitsPerSample ) || ( header.channelCount == 0 ) || ( header.channelCount > MaxChannelCount )
			|| ( header.sampleRate < MinSampleRate ) || ( header.sampleRate > MaxSampleRate ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The sound file \"%s\" has an unsupported format (%u channels, %u bits, %u Hz)", i_path,
				static_cast<unsigned int>( header.channelCount ), static_cast<unsigned int>( header.bitsPerSample ),
				static_cast<unsigned int>( header.sampleRate ) );
			return result;
		}
		const auto bytesPerFrame = static_cast<uint32_t>( header.channelCount ) * ( BitsPerSample / 8 );
		if ( ( header.sampleDataSize == 0 ) || ( ( header.sampleDataSize % bytesPerFrame ) != 0 ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The sound file \"%s\" has %u bytes of samples, which isn't a whole number of %u-byte frames", i_path,
				static_cast<unsigned int>( header.sampleDataSize ), static_cast<unsigned int>( bytesPerFrame ) );
			return result;
		}
		// (The sum is calculated with 64 bits so that a corrupted size can't wrap around in a 32-bit build)
		if ( static_cast<uint64_t>( dataFromFile.size ) != ( static_cast<uint64_t>( sizeof( header ) ) + header.sampleDataSize ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The sound file \"%s\" is %u bytes but should be %u bytes (it may be truncated or corrupted)", i_path,
				static_cast<unsigned int>( dataFromFile.size ), static_cast<unsigned int>( sizeof( header ) + header.sampleDataSize ) );
			return result;
		}
	}

	// Create the asset
	{
		newSound = new ( std::nothrow ) cSound();
		if ( !newSound )
		{
			result = Results::OutOfMemory;
			EAE6320_ASSERTF( false, "Couldn't allocate memory for the sound" );
			Logging::OutputError( "Failed to allocate memory for the sound \"%s\"", i_path );
			return result;
		}
	}
	// Copy the samples out of the file
	// (the file's memory is freed when dataFromFile goes out of scope)
	{
		try
		{
			newSound->m_samples.resize( header.sampleDataSize / sizeof( int16_t ) );
		}
		catch ( const std::exception& )
		{
			result = Results::OutOfMemory;
			Logging::OutputError( "Failed to allocate %u bytes for the samples of the sound \"%s\"",
				static_cast<unsigned int>( header.sampleDataSize ), i_path );
			return result;
		}
		const auto* const samples = reinterpret_cast<const uint8_t*>( dataFromFile.data ) + sizeof( header );
		memcpy( newSound->m_samples.data(), samples, header.sampleDataSize );
		newSound->m_sampleRate = header.sampleRate;
		newSound->m_channelCount = header.channelCount;
	}

	return result;
}

// Implementation
//===============

// Initialize / Clean Up
//----------------------

eae6320::Audio::cSound::~cSound()
{
	// The destructor is only ever called by DecrementReferenceCount(),
	// and the audio system holds a reference to every sound that is playing,
	// so the samples can't be freed while a voice is still reading them
	EAE6320_ASSERT( m_referenceCount == 0 );
}
