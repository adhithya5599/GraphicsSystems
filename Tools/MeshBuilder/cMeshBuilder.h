/*
	This class builds meshes
*/

#ifndef EAE6320_CMESHBUILDER_H
#define EAE6320_CMESHBUILDER_H

// Includes
//=========

#include <Tools/AssetBuildLibrary/iBuilder.h>

#include <Engine/Graphics/cMesh.h>
#include <Engine/Platform/Platform.h>
#include <External/Lua/Includes.h>

// Class Declaration
//==================


namespace eae6320
{
	namespace Assets
	{
		class cMeshBuilder final : public iBuilder
		{
			// Inherited Implementation
			//=========================

		public:

			// Build
			//------

			cResult Build(const std::vector<std::string>& i_arguments) final;

			// Implementation
			//===============
			eae6320::cResult LoadAsset(const char* const i_path, uint16_t*& i_index, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, uint16_t& i_vertexCount, uint16_t& i_indexCount);
			eae6320::cResult LoadTableValues_vertex(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, uint16_t& i_vertexCount);
			eae6320::cResult LoadTableValues_index(lua_State& io_luaState, uint16_t*& i_index, uint16_t& i_indexCount);
			eae6320::cResult LoadVertexValues(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, uint16_t& i_vertexCount);
			eae6320::cResult LoadIndexValues(lua_State& io_luaState, uint16_t*& i_index, uint16_t& i_indexCount);

		private:

			// This is set by LoadVertexValues():
			// true if the source file has a normal for every vertex,
			// false if it has none (in which case Build() generates them)
			bool m_doVerticesHaveNormals = false;
		};
	}
}

#endif	// EAE6320_CSHADERBUILDER_H
