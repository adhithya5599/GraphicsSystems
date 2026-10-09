// Includes
//=========

#include "cSceneLighting.h"

#include <cstring>
#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <new>
#include <string>

// Interface
//==========

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::Lighting::cSceneLighting::Load( const char* const i_path, cSceneLighting*& o_sceneLighting )
{
	auto result = Results::Success;

	cSceneLighting* newSceneLighting = nullptr;
	// This follows the same pattern as cMesh::Load() and cEffect::Load():
	// the output is only assigned if everything succeeds,
	// and if anything fails the partially-created asset is released
	cScopeGuard scopeGuard( [&o_sceneLighting, &result, &newSceneLighting]
		{
			if ( result )
			{
				EAE6320_ASSERT( newSceneLighting != nullptr );
				o_sceneLighting = newSceneLighting;
			}
			else
			{
				if ( newSceneLighting )
				{
					newSceneLighting->DecrementReferenceCount();
					newSceneLighting = nullptr;
				}
				o_sceneLighting = nullptr;
			}
		} );

	// Load the binary data
	Platform::sDataFromFile dataFromFile;
	{
		std::string errorMessage;
		if ( !( result = Platform::LoadBinaryFile( i_path, dataFromFile, &errorMessage ) ) )
		{
			Logging::OutputError( "Failed to load the lighting file \"%s\": %s", i_path, errorMessage.c_str() );
			return result;
		}
	}

	// Validate the header.
	// The builder already validated the values themselves,
	// so the only things that can be wrong here are a file that isn't a lighting file
	// or one that was built by a different version of the builder.
	{
		if ( dataFromFile.size < sizeof( sFileHeader ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The lighting file \"%s\" is too small (%u bytes) to be a lighting file",
				i_path, static_cast<unsigned int>( dataFromFile.size ) );
			return result;
		}
		sFileHeader header;
		memcpy( &header, dataFromFile.data, sizeof( header ) );
		if ( header.magic != FileMagic )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The file \"%s\" isn't a built lighting file", i_path );
			return result;
		}
		if ( ( header.version != FileVersion ) || ( header.dataSize != sizeof( sLightingData ) ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The lighting file \"%s\" was built with a different version of LightingBuilder"
				" (file version %u with %u bytes of data, expected version %u with %u bytes); rebuild the game's assets", i_path,
				static_cast<unsigned int>( header.version ), static_cast<unsigned int>( header.dataSize ),
				static_cast<unsigned int>( FileVersion ), static_cast<unsigned int>( sizeof( sLightingData ) ) );
			return result;
		}
		if ( dataFromFile.size != ( sizeof( sFileHeader ) + sizeof( sLightingData ) ) )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The lighting file \"%s\" is %u bytes but should be %u bytes (it may be truncated or corrupted)", i_path,
				static_cast<unsigned int>( dataFromFile.size ), static_cast<unsigned int>( sizeof( sFileHeader ) + sizeof( sLightingData ) ) );
			return result;
		}
	}

	// Create the asset
	{
		newSceneLighting = new ( std::nothrow ) cSceneLighting();
		if ( !newSceneLighting )
		{
			result = Results::OutOfMemory;
			EAE6320_ASSERTF( false, "Couldn't allocate memory for the scene lighting" );
			Logging::OutputError( "Failed to allocate memory for the scene lighting" );
			return result;
		}
	}
	// Copy the data out of the file.
	// The file's memory is freed when dataFromFile goes out of scope,
	// so the asset keeps its own copy (it is only a few hundred bytes).
	{
		const auto* const data = reinterpret_cast<const uint8_t*>( dataFromFile.data ) + sizeof( sFileHeader );
		memcpy( &newSceneLighting->m_data, data, sizeof( sLightingData ) );
		// Even a valid header can't protect against a corrupted count,
		// and an out-of-range count would make Graphics read past the end of the array
		if ( newSceneLighting->m_data.pointLightCount > MaxPointLightCount )
		{
			result = Results::InvalidFile;
			Logging::OutputError( "The lighting file \"%s\" has %u point lights but the maximum is %u", i_path,
				static_cast<unsigned int>( newSceneLighting->m_data.pointLightCount ), MaxPointLightCount );
			return result;
		}
	}

	Logging::OutputMessage( "Loaded the lighting file \"%s\" (%u point lights)", i_path,
		static_cast<unsigned int>( newSceneLighting->m_data.pointLightCount ) );

	return result;
}

eae6320::cResult eae6320::Lighting::cSceneLighting::CreateDefault( cSceneLighting*& o_sceneLighting )
{
	o_sceneLighting = new ( std::nothrow ) cSceneLighting();
	if ( o_sceneLighting )
	{
		// sLightingData's default member initializers provide the default lighting
		return Results::Success;
	}
	else
	{
		EAE6320_ASSERTF( false, "Couldn't allocate memory for the scene lighting" );
		Logging::OutputError( "Failed to allocate memory for the default scene lighting" );
		return Results::OutOfMemory;
	}
}

// Implementation
//===============

// Initialize / Clean Up
//----------------------

eae6320::Lighting::cSceneLighting::~cSceneLighting()
{
	// The destructor is only ever called by DecrementReferenceCount()
	// (there is nothing else to clean up because the data is stored by value)
	EAE6320_ASSERT( m_referenceCount == 0 );
}
