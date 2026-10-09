/*
	This class builds lighting files

	It reads a human-readable Lua lighting file (e.g. MyGame_/Content/Lighting/scene.lighting),
	validates every value, and writes the binary file that Engine/Lighting loads at runtime.

	Doing the parsing and validation here (at build time) rather than in the engine (at run time) means:
		* Mistakes show up as errors in Visual Studio's Error List with the file name,
			instead of as a game that silently looks wrong
		* The engine doesn't need Lua or any parsing code; loading is a single memcpy()
*/

#ifndef EAE6320_CLIGHTINGBUILDER_H
#define EAE6320_CLIGHTINGBUILDER_H

// Includes
//=========

#include <Tools/AssetBuildLibrary/iBuilder.h>

#include <Engine/Lighting/sLightingData.h>
#include <External/Lua/Includes.h>

// Class Declaration
//==================

namespace eae6320
{
	namespace Assets
	{
		class cLightingBuilder final : public iBuilder
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

			// These read each section of the Lua table into o_lightingData.
			// Every value is optional (a missing value keeps the default from sLightingData.h),
			// but a value that is present must be valid and every key must be recognized
			// (so that a typo like "intesity" is an error instead of being silently ignored).
			cResult LoadAsset( const char* const i_path, Lighting::sLightingData& o_lightingData );
			cResult LoadAmbientLight( lua_State& io_luaState, Lighting::sAmbientLight& o_ambientLight );
			cResult LoadDirectionalLight( lua_State& io_luaState, Lighting::sDirectionalLight& o_directionalLight );
			cResult LoadSpecular( lua_State& io_luaState, Lighting::sSpecular& o_specular );
			cResult LoadPointLights( lua_State& io_luaState, Lighting::sLightingData& o_lightingData );
			cResult LoadPointLight( lua_State& io_luaState, const unsigned int i_lightNumber, Lighting::sPointLight& o_pointLight );
		};
	}
}

#endif	// EAE6320_CLIGHTINGBUILDER_H
