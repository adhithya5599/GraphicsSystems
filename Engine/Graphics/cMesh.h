#pragma once

#ifndef EAE6320_GRAPHICS_CMESH_H
#define EAE6320_GRAPHICS_CMESH_H

#include <Engine/Results/Results.h>
#include "cVertexFormat.h"

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif

#ifdef EAE6320_PLATFORM_D3D
struct ID3D11Buffer;
struct ID3D11DeviceContext;
#endif // EAE6320_PLATFORM_D3D

namespace eae6320 
{
	namespace Graphics
	{
		class cMesh
		{
			public:
				cResult InitializeGeometry();
				cResult CleanUp();

#if EAE6320_PLATFORM_D3D
				void Draw(ID3D11DeviceContext* const i_direct3dImmediateContext);
#elif EAE6320_PLATFORM_GL
				void Draw();
#endif

				//cMesh();
				//~cMesh();

			private:
#ifdef EAE6320_PLATFORM_D3D
				eae6320::Graphics::cVertexFormat* m_vertexFormat = nullptr;
				ID3D11Buffer* m_vertexBuffer = nullptr;

#elif EAE6320_PLATFORM_GL
				GLuint m_vertexBufferId = 0;
				GLuint m_vertexArrayId = 0;
#endif // EAE6320_PLATFORM_D3D
		};
	}
}

#endif