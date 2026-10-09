/*
	This file defines the layout of the geometric data
	that the CPU sends to the GPU

	These must exactly match the data passed in to vertex shader programs.
*/

#ifndef EAE6320_GRAPHICS_VERTEXBUFFERFORMATS_H
#define EAE6320_GRAPHICS_VERTEXBUFFERFORMATS_H

// Includes
//=========

#include "Configuration.h"

#include <cstdint>

#if defined( EAE6320_PLATFORM_D3D )
	#include <dxgiformat.h>
#elif defined( EAE6320_PLATFORM_GL )
	#include "OpenGL/Includes.h"
#endif

// Format Definitions
//===================

namespace eae6320
{
	namespace Graphics
	{
		namespace VertexFormats
		{
			// In our class we will only have a single vertex format for all 3D geometry ("meshes").
			// In a real game it would be more common to have several different formats
			// (with simpler/smaller formats for simpler shading
			// and more complex and bigger formats for more complicated shading).
#pragma pack(push, 1)
			struct sVertex_mesh
			{
				// POSITION
				// 3 floats == 12 bytes
				// Offset = 0
				float x, y, z;
				// COLOR
				// 4 uint8_ts == 4 bytes
				// Offset = 12
				// With lighting this (multiplied by the texture) is the surface's base color ("albedo"),
				// i.e. its color under white light
				uint8_t r = 255 , g = 255, b = 255, a = 255;
				// TEXCOORD
				// 2 floats == 8 bytes
				// Offset = 16
				float u, v;
				// NORMAL
				// 3 floats == 12 bytes
				// Offset = 24
				// The direction the surface faces at this vertex (unit length, in the mesh's local space).
				// Lighting needs it: the amount of light a surface receives depends on the angle
				// between its normal and the direction to the light.
				// It is added at the end so that the offsets of the existing elements don't change.
				// (Three floats are the simplest choice; a more compact format could pack a normal
				// into 4 bytes, but that adds encode/decode work that isn't worth it at this scale.)
				float nx = 0.0f, ny = 1.0f, nz = 0.0f;
			};
#pragma pack(pop)
			// MeshBuilder writes this struct straight into the binary mesh file
			// and cMesh::Load() reads it straight back out,
			// so any change to this struct requires every mesh to be rebuilt
			// (which happens automatically because MeshBuilder.exe changes when this header changes)
			static_assert( sizeof( sVertex_mesh ) == 36, "The vertex layout must match the D3D input layout and the GL vertex attributes" );
		}
	}
}

#endif	// EAE6320_GRAPHICS_VERTEXBUFFERFORMATS_H
