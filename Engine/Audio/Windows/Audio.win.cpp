/*
	The audio system's Windows implementation, which uses XAudio2

	How XAudio2 works, in brief:
		* A "source voice" plays buffers of samples.
			Each voice has a fixed format (channel count and sample rate), its own volume, and its own pitch,
			and it mixes its sound into the "mastering voice".
		* The mastering voice sends the mix of every voice to the speakers.
		* XAudio2 does its mixing on its own thread,
			so a buffer that has been given to a voice must stay in memory until the voice has finished with it.

	Creating a voice for every sound that is played would be wasteful,
	so this keeps a small pool of voices and reuses whichever one has finished.
	Each voice holds a reference to the cSound whose samples it is playing,
	and only releases it once XAudio2 reports that the voice has finished with the buffer
	(which is what makes it safe for a game to release a sound that is still playing).
*/

// Includes
//=========

#include "../Audio.h"

#include "ExternalLibraries.win.h"

#include "../cSound.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <Engine/Asserts/Asserts.h>
#include <Engine/Concurrency/cMutex.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Windows/Includes.h>
#include <limits>
#include <xaudio2.h>

// Static Data
//============

namespace
{
	// The most sounds that can play at the same time.
	// A shot plays at most three or four sounds at once, so this leaves lots of room.
	constexpr size_t s_maxVoiceCount = 32;

	struct sVoice
	{
		IXAudio2SourceVoice* sourceVoice = nullptr;
		// The sound whose samples the voice is playing (the voice holds a reference to it).
		// This is null when the voice is free.
		eae6320::Audio::cSound* sound = nullptr;
		// The handle of the sound that is playing (see sPlayingSound in Audio.h).
		// This is 0 if the voice is free or if the sound has been stopped and the voice is waiting for XAudio2 to let go of it.
		uint16_t generation = 0;
		// The format that the voice was created with (a voice can only play sounds in its own format)
		uint32_t sampleRate = 0;
		uint16_t channelCount = 0;
	};
	std::array<sVoice, s_maxVoiceCount> s_voices;
	// Every sound that plays gets a new generation number
	// so that a handle to a sound that has finished can't affect the next sound that plays on the same voice
	uint16_t s_lastGeneration = 0;

	IXAudio2* s_xAudio2 = nullptr;
	IXAudio2MasteringVoice* s_masteringVoice = nullptr;
	bool s_isPaused = false;

	// XAudio2 stops for good if the audio device is lost (e.g. headphones are unplugged),
	// and it reports that by calling OnCriticalError() on its own thread.
	// Nothing can be done with XAudio2 from inside that call, so it only sets a flag,
	// and from then on audio is treated as disabled
	// (otherwise the voices that were playing would never finish, and every voice would end up busy forever).
	std::atomic<bool> s_hasAudioDeviceBeenLost( false );
	bool s_hasDeviceLossBeenLogged = false;
	class cEngineCallback final : public IXAudio2EngineCallback
	{
	public:
		void STDMETHODCALLTYPE OnProcessingPassStart() final {}
		void STDMETHODCALLTYPE OnProcessingPassEnd() final {}
		void STDMETHODCALLTYPE OnCriticalError( HRESULT ) final { s_hasAudioDeviceBeenLost = true; }
	};
	cEngineCallback s_engineCallback;
	// CoUninitialize() must only be called if CoInitializeEx() succeeded
	bool s_shouldComBeUninitialized = false;

	// The game plays sounds from the application loop thread
	// and the application initializes and cleans up audio from the main thread,
	// so the voice pool is protected by a mutex
	eae6320::Concurrency::cMutex s_mutex;
}

// Helper Declarations
//====================

namespace
{
	// These must be called while s_mutex is locked

	// Returns false if there is no audio device or it has been lost (and logs that it was lost the first time)
	bool IsAudioWorking();
	// Returns true if XAudio2 has finished with the voice's buffer
	bool HasVoiceFinished( const sVoice& i_voice );
	// Releases the sound of any voice that has finished
	void FreeFinishedVoices();
	// Returns the voice that a handle refers to, or null if the sound it refers to has finished or been stopped
	sVoice* FindVoice( const eae6320::Audio::sPlayingSound& i_playingSound );
	// Finds a free voice that can play a sound with the given format
	// (creating a new voice if necessary), or returns null if every voice is busy
	sVoice* FindFreeVoice( const uint16_t i_channelCount, const uint32_t i_sampleRate );
	// Destroys a voice's source voice and releases its sound
	void DestroyVoice( sVoice& io_voice );
}

// Interface
//==========

// Playback
//---------

eae6320::Audio::sPlayingSound eae6320::Audio::Play( cSound& i_sound, const sPlaybackSettings& i_settings )
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	if ( !IsAudioWorking() )
	{
		return {};
	}

	FreeFinishedVoices();
	auto* const voice = FindFreeVoice( i_sound.GetChannelCount(), i_sound.GetSampleRate() );
	if ( !voice )
	{
		Logging::OutputMessage( "Warning: A sound wasn't played because %u sounds are already playing",
			static_cast<unsigned int>( s_maxVoiceCount ) );
		return {};
	}

	// Set up the voice
	{
		// (The voice was created with a maximum frequency ratio of XAUDIO2_DEFAULT_FREQ_RATIO)
		const auto pitch = std::clamp( i_settings.pitch,
			static_cast<float>( XAUDIO2_MIN_FREQ_RATIO ), static_cast<float>( XAUDIO2_DEFAULT_FREQ_RATIO ) );
		voice->sourceVoice->SetVolume( std::max( i_settings.volume, 0.0f ) );
		voice->sourceVoice->SetFrequencyRatio( pitch );
	}
	// Give the voice the sound's samples.
	// A voice that finished its last sound is still "started" (XAudio2 only stops a voice when it is told to),
	// so it would start playing the new samples the moment it got them;
	// it is stopped first so that it only starts below (which matters when audio is paused).
	voice->sourceVoice->Stop( 0 );
	{
		const auto& samples = i_sound.GetSamples();
		XAUDIO2_BUFFER buffer{};
		buffer.Flags = XAUDIO2_END_OF_STREAM;
		buffer.AudioBytes = static_cast<UINT32>( samples.size() * sizeof( samples[0] ) );
		buffer.pAudioData = reinterpret_cast<const BYTE*>( samples.data() );
		// A LoopLength of 0 means that the whole buffer is looped
		buffer.LoopCount = i_settings.shouldLoop ? XAUDIO2_LOOP_INFINITE : 0;
		const auto result = voice->sourceVoice->SubmitSourceBuffer( &buffer );
		if ( FAILED( result ) )
		{
			Logging::OutputError( "XAudio2 failed to accept a sound's samples (HRESULT 0x%08x)", static_cast<unsigned int>( result ) );
			return {};
		}
	}
	// The voice now holds a reference to the sound, which it keeps until XAudio2 has finished with the samples
	voice->sound = &i_sound;
	i_sound.IncrementReferenceCount();
	// Start playing (unless audio is paused, in which case it starts when audio is resumed)
	if ( !s_isPaused )
	{
		const auto result = voice->sourceVoice->Start( 0 );
		if ( FAILED( result ) )
		{
			Logging::OutputError( "XAudio2 failed to start playing a sound (HRESULT 0x%08x)", static_cast<unsigned int>( result ) );
			voice->sourceVoice->FlushSourceBuffers();
			// The sound will be released once XAudio2 reports that the buffer has been flushed
			return {};
		}
	}

	// Give this use of the voice a new generation number (0 is skipped because it means "no sound")
	s_lastGeneration = ( s_lastGeneration == std::numeric_limits<uint16_t>::max() ) ? 1 : ( s_lastGeneration + 1 );
	voice->generation = s_lastGeneration;
	sPlayingSound playingSound;
	playingSound.voiceIndex = static_cast<uint16_t>( voice - s_voices.data() );
	playingSound.generation = voice->generation;
	return playingSound;
}

void eae6320::Audio::Stop( sPlayingSound& io_playingSound )
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	if ( auto* const voice = FindVoice( io_playingSound ) )
	{
		voice->sourceVoice->Stop( 0 );
		voice->sourceVoice->FlushSourceBuffers();
		// The voice can't be reused until XAudio2 has finished with the buffer
		// (FreeFinishedVoices() will release the sound once it has),
		// but no handle refers to it anymore
		voice->generation = 0;
	}
	io_playingSound = {};
}

void eae6320::Audio::SetVolume( const sPlayingSound& i_playingSound, const float i_volume )
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	if ( auto* const voice = FindVoice( i_playingSound ) )
	{
		voice->sourceVoice->SetVolume( std::max( i_volume, 0.0f ) );
	}
}

bool eae6320::Audio::IsPlaying( const sPlayingSound& i_playingSound )
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	return IsAudioWorking() && ( FindVoice( i_playingSound ) != nullptr );
}

bool eae6320::Audio::IsEnabled()
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	return IsAudioWorking();
}

void eae6320::Audio::SetIsPaused( const bool i_isPaused )
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	if ( i_isPaused == s_isPaused )
	{
		return;
	}
	s_isPaused = i_isPaused;
	if ( !IsAudioWorking() )
	{
		return;
	}
	// Stopping a voice (without flushing it) pauses it where it is
	for ( auto& voice : s_voices )
	{
		// (A voice whose sound was stopped has no generation; it must stay stopped)
		if ( voice.sound && ( voice.generation != 0 ) )
		{
			if ( s_isPaused )
			{
				voice.sourceVoice->Stop( 0 );
			}
			else
			{
				voice.sourceVoice->Start( 0 );
			}
		}
	}
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::Audio::Initialize()
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	EAE6320_ASSERTF( !s_xAudio2, "Audio has already been initialized" );

	// XAudio2 finds the audio device using COM, so COM must be initialized on this thread
	{
		const auto result = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
		if ( SUCCEEDED( result ) )
		{
			// (S_FALSE means that COM had already been initialized on this thread,
			// but it still has to be balanced by a call to CoUninitialize())
			s_shouldComBeUninitialized = true;
		}
		else if ( result != RPC_E_CHANGED_MODE )
		{
			// (RPC_E_CHANGED_MODE means that something else already initialized COM on this thread in a different mode,
			// which is fine for XAudio2)
			Logging::OutputError( "Audio is disabled because COM couldn't be initialized (HRESULT 0x%08x)", static_cast<unsigned int>( result ) );
			return Results::Success;
		}
	}
	// Create the XAudio2 engine
	{
		const auto result = XAudio2Create( &s_xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR );
		if ( FAILED( result ) )
		{
			s_xAudio2 = nullptr;
			Logging::OutputError( "Audio is disabled because XAudio2 couldn't be created (HRESULT 0x%08x)", static_cast<unsigned int>( result ) );
			return Results::Success;
		}
	}
	// Find out if the audio device is lost
	{
		const auto result = s_xAudio2->RegisterForCallbacks( &s_engineCallback );
		if ( FAILED( result ) )
		{
			// Everything still works without this; losing the audio device just wouldn't be noticed
			Logging::OutputError( "XAudio2 couldn't register for callbacks (HRESULT 0x%08x)", static_cast<unsigned int>( result ) );
		}
	}
	// Create the mastering voice, which sends everything to the default audio device
	{
		const auto result = s_xAudio2->CreateMasteringVoice( &s_masteringVoice );
		if ( FAILED( result ) )
		{
			// This is what happens when there is no audio device
			s_masteringVoice = nullptr;
			s_xAudio2->UnregisterForCallbacks( &s_engineCallback );
			s_xAudio2->Release();
			s_xAudio2 = nullptr;
			Logging::OutputMessage( "Audio is disabled because there is no audio device (HRESULT 0x%08x)", static_cast<unsigned int>( result ) );
			return Results::Success;
		}
	}

	Logging::OutputMessage( "Initialized audio (XAudio2)" );
	return Results::Success;
}

eae6320::cResult eae6320::Audio::CleanUp()
{
	Concurrency::cMutex::cScopeLock scopeLock( s_mutex );

	// Destroying a voice waits until XAudio2 has stopped using it,
	// so after this every sound's samples are safe to free
	for ( auto& voice : s_voices )
	{
		DestroyVoice( voice );
	}
	if ( s_masteringVoice )
	{
		s_masteringVoice->DestroyVoice();
		s_masteringVoice = nullptr;
	}
	if ( s_xAudio2 )
	{
		s_xAudio2->UnregisterForCallbacks( &s_engineCallback );
		s_xAudio2->Release();
		s_xAudio2 = nullptr;
	}
	s_hasAudioDeviceBeenLost = false;
	s_hasDeviceLossBeenLogged = false;
	s_isPaused = false;
	if ( s_shouldComBeUninitialized )
	{
		CoUninitialize();
		s_shouldComBeUninitialized = false;
	}

	return Results::Success;
}

// Helper Definitions
//===================

namespace
{
	bool IsAudioWorking()
	{
		if ( !s_xAudio2 )
		{
			return false;
		}
		if ( s_hasAudioDeviceBeenLost )
		{
			if ( !s_hasDeviceLossBeenLogged )
			{
				s_hasDeviceLossBeenLogged = true;
				eae6320::Logging::OutputError( "Audio has stopped because the audio device was lost (e.g. headphones were unplugged)" );
			}
			return false;
		}
		return true;
	}

	bool HasVoiceFinished( const sVoice& i_voice )
	{
		if ( !i_voice.sound )
		{
			return true;
		}
		XAUDIO2_VOICE_STATE state;
		// (Not asking for the number of samples played makes this faster)
		i_voice.sourceVoice->GetState( &state, XAUDIO2_VOICE_NOSAMPLESPLAYED );
		return state.BuffersQueued == 0;
	}

	void FreeFinishedVoices()
	{
		for ( auto& voice : s_voices )
		{
			if ( voice.sound && HasVoiceFinished( voice ) )
			{
				voice.sound->DecrementReferenceCount();
				voice.sound = nullptr;
				voice.generation = 0;
			}
		}
	}

	sVoice* FindVoice( const eae6320::Audio::sPlayingSound& i_playingSound )
	{
		if ( !i_playingSound.IsValid() || ( i_playingSound.voiceIndex >= s_voices.size() ) )
		{
			return nullptr;
		}
		auto& voice = s_voices[i_playingSound.voiceIndex];
		if ( ( voice.generation != i_playingSound.generation ) || HasVoiceFinished( voice ) )
		{
			return nullptr;
		}
		return &voice;
	}

	sVoice* FindFreeVoice( const uint16_t i_channelCount, const uint32_t i_sampleRate )
	{
		const auto IsFree = []( const sVoice& i_voice ) { return i_voice.sourceVoice && !i_voice.sound; };
		// The best voice is a free one that already has the right format
		for ( auto& voice : s_voices )
		{
			if ( IsFree( voice ) && ( voice.channelCount == i_channelCount ) && ( voice.sampleRate == i_sampleRate ) )
			{
				return &voice;
			}
		}
		// Otherwise a new voice is needed, either in an empty slot or replacing a free voice that has a different format
		sVoice* voiceToCreate = nullptr;
		for ( auto& voice : s_voices )
		{
			if ( !voice.sourceVoice )
			{
				voiceToCreate = &voice;
				break;
			}
		}
		if ( !voiceToCreate )
		{
			for ( auto& voice : s_voices )
			{
				if ( IsFree( voice ) )
				{
					DestroyVoice( voice );
					voiceToCreate = &voice;
					break;
				}
			}
		}
		if ( !voiceToCreate )
		{
			return nullptr;
		}
		// Create a voice with the sound's format
		{
			WAVEFORMATEX format{};
			format.wFormatTag = WAVE_FORMAT_PCM;
			format.nChannels = i_channelCount;
			format.nSamplesPerSec = i_sampleRate;
			format.wBitsPerSample = 16;
			format.nBlockAlign = static_cast<WORD>( i_channelCount * ( format.wBitsPerSample / 8 ) );
			format.nAvgBytesPerSec = i_sampleRate * format.nBlockAlign;
			format.cbSize = 0;
			const auto result = s_xAudio2->CreateSourceVoice( &voiceToCreate->sourceVoice, &format, 0, XAUDIO2_DEFAULT_FREQ_RATIO );
			if ( FAILED( result ) )
			{
				voiceToCreate->sourceVoice = nullptr;
				eae6320::Logging::OutputError( "XAudio2 failed to create a voice for a sound with %u channels at %u Hz (HRESULT 0x%08x)",
					static_cast<unsigned int>( i_channelCount ), static_cast<unsigned int>( i_sampleRate ), static_cast<unsigned int>( result ) );
				return nullptr;
			}
			voiceToCreate->channelCount = i_channelCount;
			voiceToCreate->sampleRate = i_sampleRate;
		}
		return voiceToCreate;
	}

	void DestroyVoice( sVoice& io_voice )
	{
		if ( io_voice.sourceVoice )
		{
			// (DestroyVoice() doesn't return until XAudio2 has stopped using the voice and its buffers)
			io_voice.sourceVoice->DestroyVoice();
			io_voice.sourceVoice = nullptr;
		}
		if ( io_voice.sound )
		{
			io_voice.sound->DecrementReferenceCount();
			io_voice.sound = nullptr;
		}
		io_voice.generation = 0;
		io_voice.channelCount = 0;
		io_voice.sampleRate = 0;
	}
}
