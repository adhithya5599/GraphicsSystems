/*
	This file defines the layout of the constant data
	that the CPU sends to the GPU

	These must exactly match the constant buffer definitions in shader programs.
*/

#ifndef EAE6320_GRAPHICS_CONSTANTBUFFERFORMATS_H
#define EAE6320_GRAPHICS_CONSTANTBUFFERFORMATS_H

// Includes
//=========

#include "Configuration.h"

#include <cstdint>
#include <Engine/Lighting/sLightingData.h>
#include <Engine/Math/cMatrix_transformation.h>

// Format Definitions
//===================

namespace eae6320
{
	namespace Graphics
	{
		namespace ConstantBufferFormats
		{
			// Data that is constant for every frame
			struct sFrame
			{
				Math::cMatrix_transformation g_transform_worldToCamera;
				Math::cMatrix_transformation g_transform_cameraToProjected;

				float g_elapsedSecondCount_systemTime = 0.0f;
				float g_elapsedSecondCount_simulationTime = 0.0f;
				// For float4 alignment
				float padding[2];

				// The camera's position in world space (w is unused)
				// Lighting needs it to calculate specular highlights,
				// which depend on the angle between the light's reflection and the direction to the camera
				float g_cameraPosition_world[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			};
			static_assert( sizeof( sFrame ) == 160, "sFrame must match g_constantBuffer_frame in shaders.inc" );

			// Data that is constant for a single draw call
			struct sDrawCall
			{
				Math::cMatrix_transformation g_transform_localToWorld;
			};

			// Data that describes every light in the scene.
			// This is filled in by Graphics from the Lighting system's sLightingData,
			// which is laid out for authoring (separate colors and intensities, a light count, etc.),
			// whereas this struct is laid out for the GPU:
			//	* Every member is a float4 or part of a group of four scalars,
			//		which is the one layout that HLSL's cbuffer packing and GLSL's std140 rules agree on exactly
			//		(a float3 followed by a float would pack differently in the two languages)
			//	* Colors are pre-multiplied by their intensities
			//		so that the shader doesn't have to do that multiplication for every pixel
			// This must match g_constantBuffer_lighting in Engine/Content/Shaders/lighting.inc
			struct sLighting
			{
				float g_ambient_skyColor[4];	// rgb = color * intensity
				float g_ambient_groundColor[4];	// rgb = color * intensity
				float g_directional_directionToLight[4];	// xyz = unit vector pointing *toward* the light
				float g_directional_color[4];	// rgb = color * intensity

				float g_specular_intensity;
				float g_specular_shininess;
				int32_t g_pointLightCount;
				float g_padding_lighting;

				float g_pointLight_positionAndRange[Lighting::MaxPointLightCount][4];	// xyz = position, w = range
				float g_pointLight_color[Lighting::MaxPointLightCount][4];	// rgb = color * intensity
			};
			static_assert( ( sizeof( sLighting ) % 16 ) == 0, "Constant buffers are made of 16-byte registers" );
			static_assert( sizeof( sLighting ) == ( 80 + ( Lighting::MaxPointLightCount * 32 ) ), "sLighting must match lighting.inc" );
		}
	}
}

#endif	// EAE6320_GRAPHICS_CONSTANTBUFFERFORMATS_H
