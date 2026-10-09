/*
	This file defines the lighting data of a scene
	and the layout of a built (binary) lighting file

	It is shared by two programs:
		* Tools/LightingBuilder fills these structs from a human-readable Lua file
			and writes them to a binary file
		* Engine/Lighting (cSceneLighting) reads the binary file straight back into the same structs
	Because both sides use this one header the binary layout can't get out of sync between them
	(the same reason MeshBuilder and cMesh share Graphics/VertexFormats.h).

	Note that this is pure data with no knowledge of the GPU:
	Graphics is responsible for converting it into the constant buffer layout that shaders use
	(see ConstantBufferFormats::sLighting).
*/

#ifndef EAE6320_LIGHTING_SLIGHTINGDATA_H
#define EAE6320_LIGHTING_SLIGHTINGDATA_H

// Includes
//=========

#include <cstdint>
#include <Engine/Math/sVector.h>
#include <type_traits>

// Constants
//==========

namespace eae6320
{
	namespace Lighting
	{
		// Shaders loop over a fixed-size array of point lights,
		// so the maximum must be known at compile time.
		// (If you change this you must also change EAE6320_LIGHTING_MAXPOINTLIGHTCOUNT in Engine/Content/Shaders/lighting.inc)
		constexpr unsigned int MaxPointLightCount = 4;

		// A built lighting file starts with this header so that the engine can detect
		// a file that isn't a lighting file, or one that was built with an older version of this struct
		// (instead of silently reading garbage)
		constexpr uint32_t FileMagic = 0x5448474C;	// The bytes "LGHT"
		// Increment this whenever any struct in this file changes
		constexpr uint32_t FileVersion = 1;
	}
}

// Struct Declarations
//====================

namespace eae6320
{
	namespace Lighting
	{
		// A color is stored separately from its intensity:
		// the color says "what hue" (each channel in [0,1])
		// and the intensity says "how bright" (any non-negative number).
		// This makes it easy to make a light brighter or dimmer without changing its hue.
		struct sColor
		{
			float r = 1.0f, g = 1.0f, b = 1.0f;
		};

		// Ambient light approximates all of the light that bounces around a scene
		// (without it anything facing away from every light would be pitch black).
		// This is a "hemisphere" ambient: surfaces facing up get the sky color,
		// surfaces facing down get the ground color, and everything in between gets a blend.
		// That is much cheaper than real bounce lighting but reads far better than a single flat color.
		struct sAmbientLight
		{
			sColor skyColor{ 0.55f, 0.65f, 0.80f };
			sColor groundColor{ 0.30f, 0.25f, 0.20f };
			float intensity = 0.35f;
		};

		// A directional light is infinitely far away (like the sun),
		// so it hits every surface from the same direction.
		struct sDirectionalLight
		{
			// The direction the light travels (from the light toward the scene), in world space.
			// It is normalized by LightingBuilder.
			Math::sVector direction{ 0.5f, -1.0f, -0.6f };
			sColor color{ 1.0f, 0.95f, 0.85f };
			float intensity = 1.0f;
		};

		// A point light shines equally in all directions from a single position (like a light bulb)
		// and fades out with distance, reaching exactly zero at its range
		struct sPointLight
		{
			Math::sVector position;	// In world space
			sColor color;
			float intensity = 1.0f;
			float range = 5.0f;	// Distance at which the light has faded to nothing
		};

		// Specular highlights are the shiny reflections of lights.
		// Strictly speaking these describe a surface (a material) rather than a light,
		// but the engine doesn't have materials yet, so for now one setting applies to every surface.
		struct sSpecular
		{
			float intensity = 0.25f;	// 0 = completely matte
			float shininess = 32.0f;	// The Blinn-Phong exponent: higher = smaller, sharper highlights
		};

		// All of the lighting in a scene
		struct sLightingData
		{
			sAmbientLight ambient;
			sDirectionalLight directional;
			sSpecular specular;
			uint32_t pointLightCount = 0;
			sPointLight pointLights[MaxPointLightCount];
		};
		// The struct is copied to and from files with memcpy()/fwrite(),
		// which is only valid for types that are "trivially copyable" (no pointers, virtual functions, etc.)
		static_assert( std::is_trivially_copyable_v<sLightingData>, "sLightingData is written to and read from binary files" );

		// A built lighting file is this header followed by an sLightingData
		struct sFileHeader
		{
			uint32_t magic = FileMagic;
			uint32_t version = FileVersion;
			// The size of the sLightingData that follows,
			// which catches a struct layout change that someone forgot to bump FileVersion for
			uint32_t dataSize = static_cast<uint32_t>( sizeof( sLightingData ) );
		};
	}
}

#endif	// EAE6320_LIGHTING_SLIGHTINGDATA_H
