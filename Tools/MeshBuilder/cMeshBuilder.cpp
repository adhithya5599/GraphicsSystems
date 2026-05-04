// Includes
//=========

#include "cMeshBuilder.h"

#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Tools/AssetBuildLibrary/Functions.h>
#include <fstream>

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
        const auto value = lua_tonumber(&io_luaState, -1);
        i_indexCount = static_cast<uint16_t>(value);
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
    float x = 0.0, y = 0.0, z = 0.0;
    uint8_t r = 0, g = 0, b = 0, a = 0;
    i_vertex = new eae6320::Graphics::VertexFormats::sVertex_mesh[i_vertexCount];
    if (i_vertexCount > 0)
    {
        for (uint16_t i = 1; i <= i_vertexCount; ++i)
        {
            lua_pushinteger(&io_luaState, i);
            lua_gettable(&io_luaState, -2);
            eae6320::cScopeGuard scopeGuard_popValue([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });

            constexpr auto* const keyX = "x";
            constexpr auto* const keyY = "y";
            constexpr auto* const keyZ = "z";

            constexpr auto* const keyR = "r";
            constexpr auto* const keyG = "g";
            constexpr auto* const keyB = "b";
            constexpr auto* const keyA = "a";

            lua_pushstring(&io_luaState, keyX);
            {
                constexpr int currentIndexOfTable = -2;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popX([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyX);
                return result;
            }

            {
                const auto value = lua_tonumber(&io_luaState, -1);
                x = static_cast<float>(value);
            }

            lua_pushstring(&io_luaState, keyY);
            {
                constexpr int currentIndexOfTable = -3;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popY([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyY);
                return result;
            }

            {
                const auto value = lua_tonumber(&io_luaState, -1);
                y = static_cast<float>(value);
            }

            lua_pushstring(&io_luaState, keyZ);
            {
                constexpr int currentIndexOfTable = -4;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popZ([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyZ);
                return result;
            }

            {
                const auto value = lua_tonumber(&io_luaState, -1);
                z = static_cast<float>(value);
            }

            lua_pushstring(&io_luaState, keyR);
            {
                constexpr int currentIndexOfTable = -5;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popR([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyR);
                return result;
            }
            {
                const auto value = lua_tonumber(&io_luaState, -1) * 255.0f;
                r = static_cast<uint8_t>(value);
            }

            lua_pushstring(&io_luaState, keyG);
            {
                constexpr int currentIndexOfTable = -6;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popG([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyG);
                return result;
            }
            {
                const auto value = lua_tonumber(&io_luaState, -1) * 255.0f;
                g = static_cast<uint8_t>(value);
            }

            lua_pushstring(&io_luaState, keyB);
            {
                constexpr int currentIndexOfTable = -7;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popB([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyB);
                return result;
            }
            {
                const auto value = lua_tonumber(&io_luaState, -1) * 255.0f;
                b = static_cast<uint8_t>(value);
            }

            lua_pushstring(&io_luaState, keyA);
            {
                constexpr int currentIndexOfTable = -8;
                lua_gettable(&io_luaState, currentIndexOfTable);
            }
            eae6320::cScopeGuard scopeGuard_popA([&io_luaState]
                {
                    lua_pop(&io_luaState, 1);
                });
            if (lua_isnil(&io_luaState, -1))
            {
                result = eae6320::Results::InvalidFile;
                OutputErrorMessageWithFileInfo(m_path_source, "No value was found %s for in asset table", keyA);
                return result;
            }
            {
                const auto value = lua_tonumber(&io_luaState, -1) * 255.0f;
                a = static_cast<uint8_t>(value);
            }

            i_vertex[i - 1] = { x, y, z, r, g, b, a };
        }
    }
    else
    {
        result = eae6320::Results::InvalidFile;
        OutputErrorMessageWithFileInfo(m_path_source, "The asset table doesn't have any ordered values");
        return result;
    }

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
            i_index[i - 1] = static_cast<uint16_t>(value);
        }
    }

    return result;
}