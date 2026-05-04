/*
	This class builds textures
*/

#ifndef EAE6320_CTEXTUREBUILDER_H
#define EAE6320_CTEXTUREBUILDER_H

// Includes
//=========
#include <Tools/AssetBuildLibrary/iBuilder.h>
#include <Engine/Platform/Platform.h>

namespace eae6320
{
	namespace Assets
	{
		//This is used to load image files
		struct sImageData
		{
			uint8_t* pixels = nullptr;
			uint32_t width = 0;
			uint32_t height = 0;
			uint8_t componentsPerPixel = 0; //3 for RGB, 4 for RGBA
			sImageData() = default;
			sImageData(const sImageData& io_movedFrom)
				: pixels(io_movedFrom.pixels), width(io_movedFrom.width), height(io_movedFrom.height),
				componentsPerPixel(io_movedFrom.componentsPerPixel)
			{
			}
			~sImageData()
			{
				if (pixels)
				{
					free(pixels);
					pixels = nullptr;
				}
				width = 0;
				height = 0;
				componentsPerPixel = 0;
			}
		};

		class cTextureBuilder final : public iBuilder
		{

		public:
			cResult Build(const std::vector<std::string>& i_arguments) final;

		private:
		};
	}
}

#endif	// EAE6320_CTEXTUREBUILDER_H