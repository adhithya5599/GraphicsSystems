#pragma once

#ifndef EAE6320_GRAPHICS_CTEXTURE_H
#define EAE6320_GRAPHICS_CTEXTURE_H

#include <Engine/Assets/ReferenceCountedAssets.h>
#include <Engine/Results/Results.h>
#include <string>
#include <cstdint>

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif // EAE6320_PLATFORM_GL


#ifdef EAE6320_PLATFORM_D3D
struct ID3D11ShaderResourceView;
#endif // EAE6320_PLATFORM_D3D

namespace eae6320
{
	namespace Texture
	{
		class cTexture
		{
		public:
			// Factory function to initialize the texture object
			static cResult Load(cTexture*& o_texture, const std::string& i_texturePath);
			
			void Bind(unsigned int i_textureUnit) const;
			static void Unbind(unsigned int i_textureUnit);

			EAE6320_ASSETS_DECLAREREFERENCECOUNTINGFUNCTIONS();
			EAE6320_ASSETS_DECLAREDELETEDREFERENCECOUNTEDFUNCTIONS(cTexture);

		private:
			uint8_t* m_pixels = nullptr;
			uint32_t m_width = 0;
			uint32_t m_height = 0;
			uint8_t m_componentsPerPixel = 0;

			EAE6320_ASSETS_DECLAREREFERENCECOUNT();
#ifdef EAE6320_PLATFORM_D3D
			ID3D11ShaderResourceView* m_textureView = nullptr;
#elif EAE6320_PLATFORM_GL
			GLuint m_textureId = 0;
#endif // EAE6320_PLATFORM_D3D

			cResult InitializeTexture();
			cResult CleanUp();
			cTexture() = default;
			~cTexture();
		};
	}
}

#endif	// EAE6320_GRAPHICS_CTEXTURE_H