#pragma once

#ifndef EAE6320_GRAPHICS_CMESH_H
#define EAE6320_GRAPHICS_CMESH_H

#include "cVertexFormat.h"
#include "sContext.h"
#include "VertexFormats.h"

#include <Engine/Results/Results.h>
#include <Engine/Assets/ReferenceCountedAssets.h>
#include <string>
#include <cstdint>

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif

#ifdef EAE6320_PLATFORM_D3D
struct ID3D11Buffer;
#endif // EAE6320_PLATFORM_D3D

namespace eae6320 
{
	namespace Graphics
	{
		class cMesh
		{
			public:
				//Factory function to initialize the mesh object
				static cResult Load(cMesh*& o_mesh, const std::string& i_vertexMeshPath);

				void Draw();

				EAE6320_ASSETS_DECLAREREFERENCECOUNTINGFUNCTIONS();
				EAE6320_ASSETS_DECLAREDELETEDREFERENCECOUNTEDFUNCTIONS(cMesh);
			
		private:
				unsigned int m_indexCount = 0;
				EAE6320_ASSETS_DECLAREREFERENCECOUNT();
#ifdef EAE6320_PLATFORM_D3D
				cVertexFormat* m_vertexFormat = nullptr;
				ID3D11Buffer* m_vertexBuffer = nullptr;
				ID3D11Buffer* m_indexBuffer = nullptr;

#elif EAE6320_PLATFORM_GL
				GLuint m_vertexBufferId = 0;
				GLuint m_vertexArrayId = 0;
				GLuint m_indexBufferId = 0;
#endif // EAE6320_PLATFORM_D3D
				cResult InitializeGeometry(VertexFormats::sVertex_mesh i_vertexData[], const unsigned int i_vertexCount, uint16_t i_indexData[]);
				cResult CleanUp();

				cMesh(const unsigned int i_indexCount)
					: m_indexCount(i_indexCount)
				{
				}
				~cMesh();
		};
	}
}

#endif