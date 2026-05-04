#include "cTexture.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>

eae6320::cResult eae6320::Texture::cTexture::Load(cTexture*& o_texture, const std::string& i_texturePath)
{
	Platform::sDataFromFile dataFromFile;
	auto result = Results::Success;
	{
		std::string errorMessage;
		if (!(result = Platform::LoadBinaryFile(i_texturePath.c_str(), dataFromFile, &errorMessage)))
		{
			EAE6320_ASSERTF(false, "Couldn't load the texture data from the binary file");
			Logging::OutputError("Failed to read from binary file with error %s", errorMessage);
			return result;
		}
	}

	uint32_t width = 0;
	uint32_t height = 0;
	uint8_t componentsPerPixel = 0;
	uint8_t* pixels = nullptr;
	auto currentOffset = reinterpret_cast<uintptr_t>(dataFromFile.data);
	const auto finalOffset = currentOffset + dataFromFile.size;
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

		newTexture->m_pixels = new uint8_t[static_cast<size_t>(pixelDataSize)];
		
		if(!pixels)
		{
			result = Results::OutOfMemory;
			EAE6320_ASSERTF(false, "Failed to allocate memory for texture pixel data");
			Logging::OutputError("Failed to allocate memory for texture pixel data");
			return result;
		}
		memcpy(newTexture->m_pixels, pixels, static_cast<size_t>(pixelDataSize));
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
	auto result = Results::Success;
	if (m_pixels)
	{
		delete[] m_pixels;
		m_pixels = nullptr;
	}
	m_width = 0;
	m_height = 0;
	m_componentsPerPixel = 0;
	
	return result;
}