#include "cTexture.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

// Helper Declarations
//====================

namespace
{
	// The textures are authored in sRGB, where the numbers aren't proportional to brightness.
	// Averaging sRGB values directly makes the smaller mip levels darker than the original
	// (e.g. the average of black and white should be a mid gray that is about 186 in sRGB, not 128),
	// so the averaging is done in linear space using these lookup tables.
	struct sSrgbTables
	{
		std::array<float, 256> srgbToLinear;
		sSrgbTables()
		{
			for ( size_t i = 0; i < srgbToLinear.size(); ++i )
			{
				srgbToLinear[i] = std::pow( static_cast<float>( i ) / 255.0f, 2.2f );
			}
		}
		uint8_t ConvertLinearToSrgb( const float i_linear ) const
		{
			const auto srgb = std::pow( std::clamp( i_linear, 0.0f, 1.0f ), 1.0f / 2.2f );
			return static_cast<uint8_t>( ( srgb * 255.0f ) + 0.5f );
		}
	};
	const sSrgbTables s_srgbTables;
}

eae6320::cResult eae6320::Texture::cTexture::Load(cTexture*& o_texture, const std::string& i_texturePath)
{
	Platform::sDataFromFile dataFromFile;
	auto result = Results::Success;
	{
		std::string errorMessage;
		if (!(result = Platform::LoadBinaryFile(i_texturePath.c_str(), dataFromFile, &errorMessage)))
		{
			EAE6320_ASSERTF(false, "Couldn't load the texture data from the binary file");
			Logging::OutputError("Failed to read from binary file with error %s", errorMessage.c_str());
			return result;
		}
	}

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t componentsPerPixel = 0;
	uint8_t* pixels = nullptr;
	auto currentOffset = reinterpret_cast<uintptr_t>(dataFromFile.data);
	cTexture* newTexture = nullptr;
	cScopeGuard scopeGuard([&o_texture, &result, &newTexture]
		{
			if (result)
			{
				EAE6320_ASSERT(newTexture != nullptr);
				o_texture = newTexture;
			}
			else
			{
				if (newTexture)
				{
					newTexture->DecrementReferenceCount();
					newTexture = nullptr;
				}
			}
		});

	// The header is the width, height, and number of components per pixel
	constexpr size_t headerSize = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
	if (dataFromFile.size < headerSize)
	{
		result = Results::InvalidFile;
		EAE6320_ASSERTF(false, "The texture file %s is too small", i_texturePath.c_str());
		Logging::OutputError("The texture file %s is too small to be a texture", i_texturePath.c_str());
		return result;
	}

	//Read the texture data
	{
		memcpy(&width, reinterpret_cast<void*>(currentOffset), sizeof(uint32_t));
	}
	{
		currentOffset += sizeof(uint32_t);
		memcpy(&height, reinterpret_cast<void*>(currentOffset), sizeof(uint32_t));
	}
	{
		currentOffset += sizeof(uint32_t);
		memcpy(&componentsPerPixel, reinterpret_cast<void*>(currentOffset), sizeof(uint8_t));
	}
	
	const uint64_t pixelDataSize = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) *
		static_cast<size_t>(componentsPerPixel);
	{
		currentOffset += sizeof(uint8_t);
		pixels = reinterpret_cast<uint8_t*>(currentOffset);
	}

	{
		newTexture = new cTexture();
		newTexture->m_width = width;
		newTexture->m_height = height;
		newTexture->m_componentsPerPixel = componentsPerPixel;

		// The file must contain all of the pixels that its header says it does
		if ((width == 0) || (height == 0) || (componentsPerPixel != 4) || ((headerSize + pixelDataSize) > dataFromFile.size))
		{
			result = Results::InvalidFile;
			EAE6320_ASSERTF(false, "The texture file %s is invalid", i_texturePath.c_str());
			Logging::OutputError("The texture file %s is invalid (it must have 4 components per pixel and all of its pixel data)",
				i_texturePath.c_str());
			return result;
		}

		newTexture->GenerateMipLevels(pixels);
	}

	if (!(result = newTexture->InitializeTexture()))
	{
		EAE6320_ASSERTF(false, "Couldn't initialize the texture");
		Logging::OutputError("Failed to initialize the texture");
		return result;
	}

	return result;
}

eae6320::Texture::cTexture::~cTexture()
{
	EAE6320_ASSERT(m_referenceCount == 0);
	const auto result = CleanUp();
	EAE6320_ASSERT(result);
}

eae6320::cResult eae6320::Texture::cTexture::CleanUp()
{
	auto result = CleanUpTexture();
	if (m_pixels)
	{
		delete[] m_pixels;
		m_pixels = nullptr;
	}
	m_width = 0;
	m_height = 0;
	m_componentsPerPixel = 0;
	m_mipLevels.clear();
	
	return result;
}

// Implementation
//===============

void eae6320::Texture::cTexture::GenerateMipLevels( const uint8_t* const i_fullSizePixels )
{
	constexpr size_t componentCount = 4;
	EAE6320_ASSERT( m_componentsPerPixel == componentCount );

	// Work out the size of every level first so that all of them can be stored in one allocation
	m_mipLevels.clear();
	size_t totalSize = 0;
	{
		auto width = m_width;
		auto height = m_height;
		while ( true )
		{
			m_mipLevels.push_back( sMipLevel{ width, height, totalSize } );
			totalSize += static_cast<size_t>( width ) * height * componentCount;
			if ( ( width == 1 ) && ( height == 1 ) )
			{
				break;
			}
			width = std::max( width / 2, 1u );
			height = std::max( height / 2, 1u );
		}
	}
	m_pixels = new uint8_t[totalSize];
	memcpy( m_pixels, i_fullSizePixels, static_cast<size_t>( m_width ) * m_height * componentCount );

	// Each level is the previous level shrunk by half:
	// every new texel is the average of the (up to) 2x2 texels that it covers
	for ( size_t levelIndex = 1; levelIndex < m_mipLevels.size(); ++levelIndex )
	{
		const auto& source = m_mipLevels[levelIndex - 1];
		const auto& destination = m_mipLevels[levelIndex];
		const auto* const sourcePixels = m_pixels + source.offset;
		auto* const destinationPixels = m_pixels + destination.offset;
		for ( uint32_t y = 0; y < destination.height; ++y )
		{
			for ( uint32_t x = 0; x < destination.width; ++x )
			{
				// (When a dimension is already 1 the same row or column is used twice)
				const uint32_t sourceXs[] = { std::min( x * 2, source.width - 1 ), std::min( ( x * 2 ) + 1, source.width - 1 ) };
				const uint32_t sourceYs[] = { std::min( y * 2, source.height - 1 ), std::min( ( y * 2 ) + 1, source.height - 1 ) };
				float sums[componentCount] = {};
				for ( const auto sourceY : sourceYs )
				{
					for ( const auto sourceX : sourceXs )
					{
						const auto* const texel = sourcePixels + ( ( ( static_cast<size_t>( sourceY ) * source.width ) + sourceX ) * componentCount );
						// The pixels are stored as blue, green, red, alpha:
						// the colors are averaged in linear space but alpha (which isn't sRGB) is averaged directly
						for ( size_t i = 0; i < 3; ++i )
						{
							sums[i] += s_srgbTables.srgbToLinear[texel[i]];
						}
						sums[3] += static_cast<float>( texel[3] ) / 255.0f;
					}
				}
				auto* const texel = destinationPixels + ( ( ( static_cast<size_t>( y ) * destination.width ) + x ) * componentCount );
				for ( size_t i = 0; i < 3; ++i )
				{
					texel[i] = s_srgbTables.ConvertLinearToSrgb( sums[i] * 0.25f );
				}
				texel[3] = static_cast<uint8_t>( ( sums[3] * 0.25f * 255.0f ) + 0.5f );
			}
		}
	}
}
