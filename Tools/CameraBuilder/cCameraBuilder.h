#pragma once

// Includes
//=========

#include <Tools/AssetBuildLibrary/iBuilder.h>

// Class Declaration
//==================

namespace eae6320::Assets
{
	class cCameraBuilder final : public iBuilder
	{
		// Inherited Implementation
		//=========================

	private:

		// Build
		//------

		cResult Build(const std::vector<std::string>& i_arguments) final;
	};
}
