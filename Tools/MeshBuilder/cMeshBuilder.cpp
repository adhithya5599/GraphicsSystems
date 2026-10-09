// Includes
//=========

#include "cMeshBuilder.h"

#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Tools/AssetBuildLibrary/Functions.h>
#include <cmath>
#include <fstream>
#include <limits>
#include <vector>

// Static Data
//============

namespace
{
    // The binary mesh format stores the vertex count, the index count, and every index as a uint16_t.
    // Values that don't fit must be rejected:
    // a static_cast would silently wrap them and the mesh would be built (and rendered) incorrectly.
    constexpr auto s_maxUint16 = std::numeric_limits<uint16_t>::max();
    constexpr unsigned int s_vertexCountPerTriangle = 3;

    // Colors are authored in [0,1] and stored as uint8_t.
    // Casting a value above 255 to uint8_t is undefined behavior
    // (on MSVC it wraps, so e.g. a channel of 5.0 became 251 instead of 255),
    // so out-of-range values are clamped first.
    uint8_t ConvertColorChannelToUint8(const lua_Number i_value)
    {
        const auto clampedValue = (i_value < 0.0) ? 0.0 : ((i_value > 1.0) ? 1.0 : i_value);
        return static_cast<uint8_t>(clampedValue * 255.0);
    }

    // This generates a normal for every vertex of a mesh whose source file doesn't have any.
    // Each triangle contributes its face normal to each of its three vertices,
    // weighted by the triangle's area (big triangles influence the result more than slivers),
    // and then every vertex's sum is normalized.
    // Normals are accumulated per vertex *index*, not per position:
    // the exporters split a vertex wherever its attributes differ (e.g. at a hard edge),
    // so accumulating per index keeps hard edges hard instead of smoothing across them.
    void GenerateNormals(eae6320::Graphics::VertexFormats::sVertex_mesh* const io_vertices, const uint16_t i_vertexCount,
        const uint16_t* const i_indices, const uint16_t i_indexCount)
    {
        struct sNormalSum { double x = 0.0, y = 0.0, z = 0.0; };
        std::vector<sNormalSum> normalSums(i_vertexCount);
        for (unsigned int i = 0; (i + 2) < i_indexCount; i += s_vertexCountPerTriangle)
        {
            const auto& v0 = io_vertices[i_indices[i + 0]];
            const auto& v1 = io_vertices[i_indices[i + 1]];
            const auto& v2 = io_vertices[i_indices[i + 2]];
            const double e1x = v1.x - v0.x, e1y = v1.y - v0.y, e1z = v1.z - v0.z;
            const double e2x = v2.x - v0.x, e2y = v2.y - v0.y, e2z = v2.z - v0.z;
            // The cross product's direction is the face normal and its length is twice the triangle's area.
            // The index order in a mesh file is the order the exporters write
            // (the Maya exporter's index_0, index_2, index_1),
            // and with that order ( v1 - v0 ) x ( v2 - v0 ) points out of the front of the triangle.
            const double fx = (e1y * e2z) - (e1z * e2y);
            const double fy = (e1z * e2x) - (e1x * e2z);
            const double fz = (e1x * e2y) - (e1y * e2x);
            for (unsigned int j = 0; j < s_vertexCountPerTriangle; ++j)
            {
                auto& sum = normalSums[i_indices[i + j]];
                sum.x += fx; sum.y += fy; sum.z += fz;
            }
        }
        for (uint16_t i = 0; i < i_vertexCount; ++i)
        {
            const auto& sum = normalSums[i];
            const auto length = std::sqrt((sum.x * sum.x) + (sum.y * sum.y) + (sum.z * sum.z));
            auto& vertex = io_vertices[i];
            if (length > 1.0e-12)
            {
                vertex.nx = static_cast<float>(sum.x / length);
                vertex.ny = static_cast<float>(sum.y / length);
                vertex.nz = static_cast<float>(sum.z / length);
            }
            else
            {
                // A vertex that isn't used by any (non-degenerate) triangle never gets drawn,
                // so any valid normal is fine
                vertex.nx = 0.0f; vertex.ny = 1.0f; vertex.nz = 0.0f;
            }
        }
    }
}

// Inherited Implementation
//=========================

// Build
//------

eae6320::cResult eae6320::Assets::cMeshBuilder::Build(const std::vector<std::string>& i_arguments)
{
	auto result = Results::Success;

    uint16_t* indexData = nullptr;
    eae6320::Graphics::VertexFormats::sVertex_mesh* vertexData = nullptr;
    uint16_t vertexCount;
    uint16_t indexCount;
    
    {
        if (!(result = LoadAsset(m_path_source, indexData, vertexData, vertexCount, indexCount)))
        {
            EAE6320_ASSERTF(false, "Couldn't load the mesh data from the lua file");
            OutputErrorMessageWithFileInfo(m_path_source, "Failed to read the mesh data from the lua file");
            return result;
        }
    }

    // Every index must refer to a vertex that exists
    for (uint16_t i = 0; i < indexCount; ++i)
    {
        if (indexData[i] >= vertexCount)
        {
            result = eae6320::Results::InvalidFile;
            OutputErrorMessageWithFileInfo(m_path_source, "Index #%u (%u) refers to a vertex that doesn't exist (the vertex count is %u)",
                static_cast<unsigned int>(i + 1), static_cast<unsigned int>(indexData[i]), static_cast<unsigned int>(vertexCount));
            return result;
        }
    }

    // Meshes whose source files don't have normals (e.g. any .mayamesh exported before normals were added)
    // still need them for lighting
    if (!m_doVerticesHaveNormals)
    {
        GenerateNormals(vertexData, vertexCount, indexData, indexCount);
    }

    {
        std::ofstream fout(m_path_target, std::ofstream::binary | std::ofstream::out);
        fout.write(reinterpret_cast<const char*>(&vertexCount), sizeof(uint16_t));
        fout.write(reinterpret_cast<const char*>(&indexCount), sizeof(uint16_t));
        fout.write(reinterpret_cast<const char*>(vertexData), sizeof(eae6320::Graphics::VertexFormats::sVertex_mesh) * vertexCount);
        fout.write(reinterpret_cast<const char*>(indexData), sizeof(uint16_t) * indexCount);

        fout.close();
    }

	return result;
}

eae6320::cResult eae6320::Assets::cMeshBuilder::LoadAsset(const char* const i_path, uint16_t*& i_index, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, uint16_t& i_vertexCount, uint16_t& i_indexCount)
{
    auto result = eae6320::Results::Success;

    // Create a new Lua state
    lua_State* luaState = nullptr;
    eae6320::cScopeGuard scopeGuard_onExit([&luaState]
        {
            if (luaState)
            {
                EAE6320_ASSERT(lua_gettop(luaState) == 0);

                lua_close(luaState);
                luaState = nullptr;
            }
        });
    {
        luaState = luaL_newstate();
        if (!luaState)
        {
            result = eae6320::Results::OutOfMemory;
            OutputErrorMessageWithFileInfo(m_path_source, "Failed to create a new Lua state");
            return result;
        }
    }

    //Load the asset file
    const auto stackTopBeforeLoad = lua_gettop(luaState);
    {
        const auto luaResult = luaL_loadfile(luaState, i_path);
        if (luaResult != LUA_OK)
        {
            result = eae6320::Results::Failure;
            OutputErrorMessageWithFileInfo(m_path_source, lua_tostring(luaState, -1));
            lua_pop(luaState, 1);
            return result;
        }
    }

    // Execute the "chunk", which should load the asset
    {
        constexpr int argumentCount = 0;
        constexpr int returnValueCount = LUA_MULTRET; // Return _everything_ that the file returns
        constexpr int noMessageHandler = 0;
        const auto luaResult = lua_pcall(luaState, argumentCount, returnValueCount, noMessageHandler);

        if (luaResult == LUA_OK)
        {
            const auto returnedValueCount = lua_gettop(luaState) - stackTopBeforeLoad;
            if (returnedValueCount == 1)
            {
                if (!lua_istable(luaState, -1))
                {
                    result = eae6320::Results::InvalidFile;
                    OutputErrorMessageWithFileInfo(m_path_source, "Asset files must return a table");
                    lua_pop(luaState, 1);
                    return result;
                }
            }
            else
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "Asset files must return a single table (instead of %d values)", returnedValueCount);
                lua_pop(luaState, returnedValueCount);
                return result;
            }
        }
        else
        {
            result = eae6320::Results::InvalidFile;
            OutputErrorMessageWithFileInfo(m_path_source, lua_tostring(luaState, -1));
            lua_pop(luaState, 1);
            return result;
        }
    }

    eae6320::cScopeGuard scopeGuard_popAssetTable([&luaState]
        {
            lua_pop(luaState, 1);
        });

    if (!(result = LoadTableValues_vertex(*luaState, i_vertex, i_vertexCount)))
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "Failed to load the vertex values from the file");
        return result;
    }

    if (!(result = LoadTableValues_index(*luaState, i_index, i_indexCount)))
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "Failed to load the index values from the file");
        return result;
    }

    return result;
}

eae6320::cResult eae6320::Assets::cMeshBuilder::LoadTableValues_vertex(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, uint16_t& i_vertexCount)
{
    auto result = eae6320::Results::Success;
    constexpr auto* const keyVertexCount = "vertexCount";
    lua_pushstring(&io_luaState, keyVertexCount);
    {
        constexpr int currentIndexOfTable = -2;
        lua_gettable(&io_luaState, currentIndexOfTable);
    }
    if (lua_isnil(&io_luaState, -1))
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyVertexCount);
        return result;
    }
    eae6320::cScopeGuard scopeGuard_popVertexCount([&io_luaState]
        {
            lua_pop(&io_luaState, 1);
        });
    {
        const auto value = lua_tonumber(&io_luaState, -1);
        if ((value < 0.0) || (value > static_cast<lua_Number>(s_maxUint16)))
        {
            result = eae6320::Results::InvalidFile;
            OutputErrorMessageWithFileInfo(m_path_source, "The mesh has %.0f vertices but the maximum is %u", value,
                static_cast<unsigned int>(s_maxUint16));
            return result;
        }
        i_vertexCount = static_cast<uint16_t>(value);
    }

    constexpr auto* const key = "vertex";
    lua_pushstring(&io_luaState, key);
    lua_gettable(&io_luaState, -3);
    eae6320::cScopeGuard scopeGuard_popTextures([&io_luaState]
        {
            lua_pop(&io_luaState, 1);
        });

    if (lua_istable(&io_luaState, -1))
    {
        if (!(result = LoadVertexValues(io_luaState, i_vertex, i_vertexCount)))
        {
            return result;
        }
    }
    else
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "The value must be a table");
        return result;
    }

    return result;
}

eae6320::cResult eae6320::Assets::cMeshBuilder::LoadTableValues_index(lua_State& io_luaState, uint16_t*& i_index, uint16_t& i_indexCount)
{
    auto result = eae6320::Results::Success;
    constexpr auto* const keyIndexCount = "triangleCount";
    lua_pushstring(&io_luaState, keyIndexCount);
    {
        constexpr int currentIndexOfTable = -2;
        lua_gettable(&io_luaState, currentIndexOfTable);
    }
    if (lua_isnil(&io_luaState, -1))
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyIndexCount);
        return result;
    }
    eae6320::cScopeGuard scopeGuard_popVertexCount([&io_luaState]
        {
            lua_pop(&io_luaState, 1);
        });
    {
        // Note that "triangleCount" is actually the number of indices (3 per triangle)
        const auto value = lua_tonumber(&io_luaState, -1);
        if ((value < 0.0) || (value > static_cast<lua_Number>(s_maxUint16)))
        {
            result = eae6320::Results::InvalidFile;
            OutputErrorMessageWithFileInfo(m_path_source, "The mesh has %.0f indices (%.0f triangles) but the maximum is %u indices (%u triangles)",
                value, value / s_vertexCountPerTriangle,
                static_cast<unsigned int>(s_maxUint16), static_cast<unsigned int>(s_maxUint16) / s_vertexCountPerTriangle);
            return result;
        }
        i_indexCount = static_cast<uint16_t>(value);
        if ((i_indexCount % s_vertexCountPerTriangle) != 0)
        {
            result = eae6320::Results::InvalidFile;
            OutputErrorMessageWithFileInfo(m_path_source, "The index count (%u) must be a multiple of %u",
                static_cast<unsigned int>(i_indexCount), s_vertexCountPerTriangle);
            return result;
        }
    }


    constexpr auto* const key = "index";
    lua_pushstring(&io_luaState, key);
    lua_gettable(&io_luaState, -3);
    eae6320::cScopeGuard scopeGuard_popTextures([&io_luaState]
        {
            lua_pop(&io_luaState, 1);
        });
    if (lua_istable(&io_luaState, -1))
    {
        if (!(result = LoadIndexValues(io_luaState, i_index, i_indexCount)))
        {
            return result;
        }
    }
    else
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "The value must be a table");
        return result;
    }

    return result;
}

eae6320::cResult eae6320::Assets::cMeshBuilder::LoadVertexValues(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, uint16_t& i_vertexCount)
{
    auto result = eae6320::Results::Success;
    if (i_vertexCount == 0)
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "The asset table doesn't have any ordered values");
        return result;
    }
    i_vertex = new eae6320::Graphics::VertexFormats::sVertex_mesh[i_vertexCount];

    // Normals are optional in the source file:
    // meshes exported before normals were added to the vertex format (e.g. every .mayamesh) don't have them,
    // and Build() generates normals for those meshes.
    // A file must be consistent, though: either every vertex has a normal or none do
    // (a mix would mean some normals were authored and some would be invented, which would silently look wrong).
    size_t vertexCountWithNormals = 0;

    for (uint16_t i = 1; i <= i_vertexCount; ++i)
    {
        // Push the vertex table
        lua_pushinteger(&io_luaState, i);
        lua_gettable(&io_luaState, -2);
        eae6320::cScopeGuard scopeGuard_popVertex([&io_luaState]
            {
                lua_pop(&io_luaState, 1);
            });
        if (!lua_istable(&io_luaState, -1))
        {
            result = eae6320::Results::InvalidFile;
            OutputErrorMessageWithFileInfo(m_path_source, "Vertex #%u must be a table", static_cast<unsigned int>(i));
            return result;
        }

        // This reads a single number from the vertex table
        // (lua_getfield() pushes the value, so it is always popped before returning)
        const auto ReadNumber = [this, &io_luaState, i](const char* const i_key, lua_Number& o_value, bool& o_wasFound) -> eae6320::cResult
        {
            lua_getfield(&io_luaState, -1, i_key);
            eae6320::cScopeGuard scopeGuard_popValue([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            o_wasFound = !lua_isnil(&io_luaState, -1);
            if (!o_wasFound)
            {
                return eae6320::Results::Success;
            }
            if (!lua_isnumber(&io_luaState, -1))
            {
                OutputErrorMessageWithFileInfo(m_path_source, "The value of %s in vertex #%u must be a number", i_key, static_cast<unsigned int>(i));
                return eae6320::Results::InvalidFile;
            }
            o_value = lua_tonumber(&io_luaState, -1);
            return eae6320::Results::Success;
        };
        const auto ReadRequiredNumber = [this, &ReadNumber, i](const char* const i_key, lua_Number& o_value) -> eae6320::cResult
        {
            bool wasFound = false;
            const auto result = ReadNumber(i_key, o_value, wasFound);
            if (result && !wasFound)
            {
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found for %s in vertex #%u", i_key, static_cast<unsigned int>(i));
                return eae6320::Results::InvalidFile;
            }
            return result;
        };

        auto& vertex = i_vertex[i - 1];

        // Position
        {
            lua_Number x = 0.0, y = 0.0, z = 0.0;
            if (!(result = ReadRequiredNumber("x", x)) || !(result = ReadRequiredNumber("y", y)) || !(result = ReadRequiredNumber("z", z)))
            {
                return result;
            }
            vertex.x = static_cast<float>(x);
            vertex.y = static_cast<float>(y);
            vertex.z = static_cast<float>(z);
        }
        // Normal (optional)
        {
            lua_Number nx = 0.0, ny = 0.0, nz = 0.0;
            bool wasFound_x = false, wasFound_y = false, wasFound_z = false;
            if (!(result = ReadNumber("nx", nx, wasFound_x)) || !(result = ReadNumber("ny", ny, wasFound_y)) || !(result = ReadNumber("nz", nz, wasFound_z)))
            {
                return result;
            }
            if (wasFound_x || wasFound_y || wasFound_z)
            {
                if (!(wasFound_x && wasFound_y && wasFound_z))
                {
                    result = eae6320::Results::InvalidFile;
                    OutputErrorMessageWithFileInfo(m_path_source, "Vertex #%u must have all of nx, ny, and nz (or none of them)", static_cast<unsigned int>(i));
                    return result;
                }
                // Exporters should write unit-length normals, but re-normalizing here is cheap
                // and protects the shaders from un-normalized input
                const auto length = std::sqrt((nx * nx) + (ny * ny) + (nz * nz));
                if (!(length > 1.0e-6))
                {
                    result = eae6320::Results::InvalidFile;
                    OutputErrorMessageWithFileInfo(m_path_source, "The normal of vertex #%u has zero length", static_cast<unsigned int>(i));
                    return result;
                }
                vertex.nx = static_cast<float>(nx / length);
                vertex.ny = static_cast<float>(ny / length);
                vertex.nz = static_cast<float>(nz / length);
                ++vertexCountWithNormals;
            }
        }
        // Texture coordinates
        {
            lua_Number u = 0.0, v = 0.0;
            if (!(result = ReadRequiredNumber("u", u)) || !(result = ReadRequiredNumber("v", v)))
            {
                return result;
            }
            vertex.u = static_cast<float>(u);
            vertex.v = static_cast<float>(v);
        }
        // Color
        {
            lua_Number r = 0.0, g = 0.0, b = 0.0, a = 0.0;
            if (!(result = ReadRequiredNumber("r", r)) || !(result = ReadRequiredNumber("g", g))
                || !(result = ReadRequiredNumber("b", b)) || !(result = ReadRequiredNumber("a", a)))
            {
                return result;
            }
            vertex.r = ConvertColorChannelToUint8(r);
            vertex.g = ConvertColorChannelToUint8(g);
            vertex.b = ConvertColorChannelToUint8(b);
            vertex.a = ConvertColorChannelToUint8(a);
        }
    }

    if ((vertexCountWithNormals != 0) && (vertexCountWithNormals != i_vertexCount))
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "%u of %u vertices have normals; either every vertex must have a normal or none of them",
            static_cast<unsigned int>(vertexCountWithNormals), static_cast<unsigned int>(i_vertexCount));
        return result;
    }
    m_doVerticesHaveNormals = (vertexCountWithNormals == i_vertexCount);

    return result;
}

eae6320::cResult eae6320::Assets::cMeshBuilder::LoadIndexValues(lua_State& io_luaState, uint16_t*& i_index, uint16_t& i_indexCount)
{
    auto result = eae6320::Results::Success;
    if (i_indexCount > 0)
    {
        i_index = new uint16_t[i_indexCount];
        for (uint16_t i = 1; i <= i_indexCount; ++i)
        {
            lua_pushinteger(&io_luaState, i);
            lua_gettable(&io_luaState, -2);

            eae6320::cScopeGuard scopeGuard_popValue([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            const auto value = lua_tonumber(&io_luaState, -1);
            if ((value < 0.0) || (value > static_cast<lua_Number>(s_maxUint16)))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "Index #%u (%.0f) doesn't fit in a uint16_t",
                    static_cast<unsigned int>(i), value);
                return result;
            }
            i_index[i - 1] = static_cast<uint16_t>(value);
        }
    }

    return result;
}