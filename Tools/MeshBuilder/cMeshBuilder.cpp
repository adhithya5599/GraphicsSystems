// Includes
//=========

#include "cMeshBuilder.h"

#include <Tools/AssetBuildLibrary/Functions.h>

// Inherited Implementation
//=========================

// Build
//------

eae6320::cResult eae6320::Assets::cMeshBuilder::Build(const std::vector<std::string>& i_arguments)
{
	auto result = Results::Success;
	if (!(result = Platform::CopyFile(m_path_source, m_path_target, false, true)))
	{
		OutputErrorMessageWithFileInfo(m_path_source, 17, "Failed to copy target \"%s\" to source", m_path_target);
		return result;
	}

	return result;
}