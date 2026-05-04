#pragma once

#ifndef E6320_GRAPHICS_EFFECT_H
#define E6320_GRAPHICS_EFFECT_H

#include "cShader.h"
#include "cRenderState.h"
#include "sContext.h"

#include <Engine/Results/Results.h>

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif

namespace eae6320
{
	namespace Graphics
	{
		class cEffect
		{
			public:
				cResult InitializeShadingData(const std::string &i_vertexShaderPath, const std::string &i_fragmentShaderPath);
				cResult CleanUp();

				cResult Initialize_platformSpecific();
				cResult CleanUp_platformSpecific();
				void Bind();

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
