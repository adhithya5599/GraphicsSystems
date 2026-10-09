/*
	A scene lighting asset holds every light in a scene

	It is loaded from a binary file that Tools/LightingBuilder built from a Lua file,
	and it is reference counted like the other assets (cMesh, cEffect, cShader):
	whoever holds a pointer to it owns a reference,
	and the asset deletes itself when the last reference is released.

	This class deliberately knows nothing about the GPU.
	Graphics owns the scene lighting and converts it into constant buffer data,
	which keeps the dependency one-way (Graphics -> Lighting)
	and means the game never has to touch the lighting system at all.
*/

#ifndef EAE6320_LIGHTING_CSCENELIGHTING_H
#define EAE6320_LIGHTING_CSCENELIGHTING_H

// Includes
//=========

#include "sLightingData.h"

#include <Engine/Assets/ReferenceCountedAssets.h>
#include <Engine/Results/Results.h>

// Class Declaration
//==================

namespace eae6320
{
	namespace Lighting
	{
		class cSceneLighting
		{
			// Interface
			//==========

		public:

			// Access
			//-------

			const sLightingData& GetData() const { return m_data; }

			// Initialize / Clean Up
			//----------------------

			// Loads a built (binary) lighting file.
			// On success o_sceneLighting holds a new reference that the caller must release with DecrementReferenceCount().
			static cResult Load( const char* const i_path, cSceneLighting*& o_sceneLighting );
			// Creates scene lighting with the default values from sLightingData.h
			// (used as a fallback so that a missing or invalid lighting file doesn't stop the game from rendering)
			static cResult CreateDefault( cSceneLighting*& o_sceneLighting );

			EAE6320_ASSETS_DECLAREDELETEDREFERENCECOUNTEDFUNCTIONS( cSceneLighting );

			// Reference Counting
			//-------------------

			EAE6320_ASSETS_DECLAREREFERENCECOUNTINGFUNCTIONS();

			// Data
			//=====

		private:

			sLightingData m_data;

			EAE6320_ASSETS_DECLAREREFERENCECOUNT();

			// Implementation
			//===============

		private:

			// Initialize / Clean Up
			//----------------------

			// The constructor and destructor are private so that the only way to create an instance is Load()/CreateDefault()
			// and the only way to destroy one is to release the last reference
			cSceneLighting() = default;
			~cSceneLighting();
		};
	}
}

#endif	// EAE6320_LIGHTING_CSCENELIGHTING_H
