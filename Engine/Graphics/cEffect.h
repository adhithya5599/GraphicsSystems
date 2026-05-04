#pragma once

#ifndef E6320_GRAPHICS_EFFECT_H
#define E6320_GRAPHICS_EFFECT_H

#include "cShader.h"
#include "cRenderState.h"

#include <Engine/Results/Results.h>

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif

#ifdef EAE6320_PLATFORM_D3D
struct ID3D11ClassInstance;
struct ID3D11DeviceContext;
#endif // EAE6320_PLATFORM_D3D


namespace eae6320
{
	namespace Graphics
	{
		class cEffect
		{
			public:
				cResult InitializeShadingData();
				cResult CleanUp();

				void Initialize_platformSpecific(cResult& o_result);
				cResult CleanUp_platformSpecific();
#ifdef EAE6320_PLATFORM_D3D
				void Bind(ID3D11DeviceContext* const i_direct3dImmediateContext);
#elif EAE6320_PLATFORM_GL
				void Bind();
#endif
			private:
				cShader* m_vertexShader = nullptr;
				cShader* m_fragmentShader = nullptr;
				cRenderState m_renderState;
#if EAE6320_PLATFORM_GL
				GLuint m_programId = 0;
#endif // EAE6320_PLATFORM_GL			
		};
	}
}

#endif // !E6320_GRAPHICS_EFFECT_H
