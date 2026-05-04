#pragma once

#ifndef EAE6320_GRAPHICS_CMESH_H
#define EAE6320_GRAPHICS_CMESH_H

#include "cVertexFormat.h"
#include "sContext.h"
#include "VertexFormats.h"

#include <Engine/Results/Results.h>

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
				cResult InitializeGeometry(VertexFormats::sVertex_mesh i_vertexData[], const unsigned int i_vertexCount,  uint16_t i_indexData[]);
				cResult CleanUp();
				cResult InitializeViews(const sInitializationParameters& i_initializationParameters);
				
				void Draw();

				cMesh(const unsigned int i_indexCount)
					: m_indexCount(i_indexCount)
				{ }

			
		private:
				unsigned int m_indexCount = 0;
#ifdef EAE6320_PLATFORM_D3D
				cVertexFormat* m_vertexFormat = nullptr;
				ID3D11Buffer* m_vertexBuffer = nullptr;
				ID3D11Buffer* m_indexBuffer = nullptr;

#elif EAE6320_PLATFORM_GL
				GLuint m_vertexBufferId = 0;
				GLuint m_vertexArrayId = 0;
				GLuint m_indexBufferId = 0;
#endif // EAE6320_PLATFORM_D3D
		};
	}
}

#endif