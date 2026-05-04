#pragma once

#ifndef E6320_GRAPHICS_EFFECT_H
#define E6320_GRAPHICS_EFFECT_H

#include "cShader.h"
#include "cRenderState.h"
#include "sContext.h"

#include <Engine/Results/Results.h>
#include <Engine/Assets/ReferenceCountedAssets.h>

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
				//Factory function to initialize the effect object
				static cResult Load(cEffect*& o_effect, const std::string& i_vertexShaderPath, const std::string& i_fragmentShaderPath);

				EAE6320_ASSETS_DECLAREREFERENCECOUNTINGFUNCTIONS();
				EAE6320_ASSETS_DECLAREDELETEDREFERENCECOUNTEDFUNCTIONS(cEffect);

				void Bind();

			private:
				EAE6320_ASSETS_DECLAREREFERENCECOUNT();
				cShader* m_vertexShader = nullptr;
				cShader* m_fragmentShader = nullptr;
				cRenderState m_renderState;
#if EAE6320_PLATFORM_GL
				GLuint m_programId = 0;
#endif // EAE6320_PLATFORM_GL	
				cResult InitializeShadingData(const std::string& i_vertexShaderPath, const std::string& i_fragmentShaderPath);
				cResult CleanUp();

				cResult Initialize_platformSpecific();
				cResult CleanUp_platformSpecific();
				cEffect() = default;
				~cEffect();
		};
	}
}

#endif // !E6320_GRAPHICS_EFFECT_H
