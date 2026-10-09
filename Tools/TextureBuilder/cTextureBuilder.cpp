#pragma once

#include "cTextureBuilder.h"
#include <Tools/AssetBuildLibrary/Functions.h>
#include <Engine/Asserts/Asserts.h>
#include <fstream>

eae6320::cResult eae6320::Assets::cTextureBuilder::Build(const std::vector<std::string>& i_arguments)
{
	auto result = Results::Success;
	// Load the source image
	eae6320::Platform::sImageData s_imageData;
	eae6320::Platform::sDataFromFile dataFromFile;
	{
		std::string errorMessage;
		if (!(result = eae6320::Platform::LoadBinaryFile(m_path_source, dataFromFile, &errorMessage)))
		{
			EAE6320_ASSERTF(false, "Couldn't load the source image");
			OutputErrorMessageWithFileInfo(m_path_source, "Failed to load the source image with error %s", errorMessage);
			return result;
		}
	}

	//Parse the image data
	{
		if (dataFromFile.data)
		{
			const uint8_t* const imageData = reinterpret_cast<uint8_t*>(dataFromFile.data);
			const uint16_t type = *reinterpret_cast<const uint16_t*>(imageData);

			if (type != 0x4D42) // 'BM' for BMP files
			{
				result = Results::InvalidFile;
				return result;
			}

			const uint32_t bitmapSize = *reinterpret_cast<const uint32_t*>(imageData + 2);
			const uint32_t pixelDataOffset = *reinterpret_cast<const uint32_t*>(imageData + 10);

			const uint32_t dibHeaderSize = *reinterpret_cast<const uint32_t*>(imageData + 14);
			const uint32_t width = *reinterpret_cast<const uint32_t*>(imageData + 18);
			const uint32_t height = *reinterpret_cast<const uint32_t*>(imageData + 22);
			const uint16_t planes = *reinterpret_cast<const uint16_t*>(imageData + 26);
			const uint16_t bitsPerPixel = *reinterpret_cast<const uint16_t*>(imageData + 28);

			if(bitsPerPixel != 24 && bitsPerPixel != 32)
			{
				result = Results::InvalidFile;
				OutputErrorMessageWithFileInfo(m_path_source,"Only 24-bit and 32-bit BMP textures are supported");
				return result;
			}

			const uint16_t compression = *reinterpret_cast<const uint16_t*>(imageData + 30);

			if (compression != 0)
			{
				result = Results::InvalidFile;
				return result;
			}

			const uint8_t componentsPerPixel = static_cast<uint8_t>(bitsPerPixel / 8);
			const uint8_t* const pixelData = imageData + pixelDataOffset;

			const uint32_t rowSize = ((width * componentsPerPixel + 3) & ~3);
			const uint32_t finalComponents = 4;

			{
				s_imageData.width = width;
				s_imageData.height = height;
				s_imageData.componentsPerPixel = finalComponents;
				//s_imageData.pixels = reinterpret_cast<uint8_t*>(malloc(width * height * componentsPerPixel));
				const size_t totalPixels = static_cast<size_t>(width) * static_cast<size_t>(height);
				const size_t pixelSize = totalPixels * static_cast<size_t>(finalComponents);

				s_imageData.pixels = new uint8_t[pixelSize];

				if (!s_imageData.pixels)
				{
					result = Results::OutOfMemory;
					return result;
				}

				const uint32_t rowStride = width * finalComponents;

				for (uint32_t y = 0; y < height; ++y)
				{
					const uint8_t* const sourceRow = pixelData + (height - 1 - y) * rowSize;
					uint8_t* const destinationRow = s_imageData.pixels + (static_cast<size_t>(y) * rowStride);

					for(uint32_t i = 0; i < width; ++i)
					{
						const uint8_t* const sourcePixel = sourceRow + i * componentsPerPixel;
						uint8_t* const destinationPixel = destinationRow + i * finalComponents;
						
						const uint8_t blue = sourcePixel[0];
						const uint8_t green = sourcePixel[1];
						const uint8_t red = sourcePixel[2];
						uint8_t alpha = 0xFF;
						
						if(componentsPerPixel == 4)
						{
							alpha = sourcePixel[3];
						}

						destinationPixel[0] = blue;
						destinationPixel[1] = green;
						destinationPixel[2] = red;
						destinationPixel[3] = alpha; // Alpha channel
					}
				}
			}
		}
	}

	// Write the texture data to the target file
	{
		std::ofstream fout(m_path_target, std::ofstream::binary | std::ofstream::out);
		fout.write(reinterpret_cast<const char*>(&s_imageData.width), sizeof(uint32_t));
		fout.write(reinterpret_cast<const char*>(&s_imageData.height), sizeof(uint32_t));
		fout.write(reinterpret_cast<const char*>(&s_imageData.componentsPerPixel), sizeof(uint8_t));
		const uint64_t pixelDataSize = static_cast<uint64_t>(s_imageData.width) * static_cast<uint64_t>(s_imageData.height) *
			static_cast<size_t>(s_imageData.componentsPerPixel);
		fout.write(reinterpret_cast<const char*>(s_imageData.pixels), pixelDataSize);
		fout.close();
	}

	return result;
}
