#pragma once

#ifndef EAE6320_GRAPHICS_CTEXTURE_H
#define EAE6320_GRAPHICS_CTEXTURE_H

#include <Engine/Assets/ReferenceCountedAssets.h>
#include <Engine/Results/Results.h>
#include <cstdint>
#include <string>
#include <vector>

#ifdef EAE6320_PLATFORM_GL
#include "OpenGL/Includes.h"
#endif // EAE6320_PLATFORM_GL


#ifdef EAE6320_PLATFORM_D3D
struct ID3D11SamplerState;
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

			// A mip map level is a copy of the texture at a smaller size
			// (each level is half the width and height of the one before it, down to 1x1).
			// When a texture is far away (or seen at a grazing angle, like a long field of grass)
			// many texels land on a single pixel; without smaller levels the GPU picks just one of them,
			// which makes the texture shimmer and sparkle as the camera moves.
			// With mip maps the GPU samples a level whose texels are about the size of a pixel instead.
			struct sMipLevel
			{
				uint32_t width = 0;
				uint32_t height = 0;
				// Where this level's pixels start in m_pixels (in bytes)
				size_t offset = 0;
			};

			// Every mip level's pixels, one level after the other
			// (they are only needed until they have been uploaded to the GPU)
			uint8_t* m_pixels = nullptr;
			std::vector<sMipLevel> m_mipLevels;
			uint32_t m_width = 0;
			uint32_t m_height = 0;
			uint8_t m_componentsPerPixel = 0;

			EAE6320_ASSETS_DECLAREREFERENCECOUNT();
#ifdef EAE6320_PLATFORM_D3D
			ID3D11ShaderResourceView* m_textureView = nullptr;
			// How the texture is sampled (repeating, filtered between mip levels).
			// Direct3D needs a sampler state object for this
			// (without one bound it uses a default that clamps instead of repeating and ignores mip maps).
			// Identical sampler states are shared by Direct3D, so every texture creating its own costs nothing extra.
			ID3D11SamplerState* m_samplerState = nullptr;
#elif EAE6320_PLATFORM_GL
			GLuint m_textureId = 0;
#endif // EAE6320_PLATFORM_D3D

			// Builds the smaller mip levels from the full-size image (which must already be at the start of m_pixels)
			void GenerateMipLevels( const uint8_t* const i_fullSizePixels );

			// Platform-specific
			cResult InitializeTexture();
			cResult CleanUpTexture();

			cResult CleanUp();
			cTexture() = default;
			~cTexture();
		};
	}
}

#endif	// EAE6320_GRAPHICS_CTEXTURE_H