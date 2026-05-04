#include "cMesh.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Logging/Logging.h>
#include <External/Lua/Includes.h>

namespace
{
    eae6320::cResult LoadAsset(const char* const i_path, uint16_t*& i_index, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, unsigned int& i_vertexCount, unsigned int& i_indexCount);
    eae6320::cResult LoadTableValues_vertex(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, unsigned int& i_vertexCount);
    eae6320::cResult LoadTableValues_index(lua_State& io_luaState, uint16_t*& i_index, unsigned int& i_indexCount);
    eae6320::cResult LoadVertexValues(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, unsigned int& i_vertexCount);
    eae6320::cResult LoadIndexValues(lua_State& io_luaState, uint16_t*& i_index, unsigned int& i_indexCount);
}



eae6320::cResult eae6320::Graphics::cMesh::Load(cMesh*& o_mesh, const std::string& i_vertexMeshPath)
{
    uint16_t* indexData = nullptr;
    eae6320::Graphics::VertexFormats::sVertex_mesh* vertexData = nullptr;
    unsigned int vertexCount;
    unsigned int indexCount;

    //constexpr auto* const vertexpath = "data/Meshes/geometry.lua";
    auto result = Results::Success;
    
    {
        if (!(result = LoadAsset(i_vertexMeshPath.c_str(), indexData, vertexData, vertexCount, indexCount)))
        {
            EAE6320_ASSERTF(false, "Couldn't load the mesh data from the lua file");
            Logging::OutputError("Failed to read the mesh data from the lua file");
            return result;
        }
    }

    cMesh* newMesh = nullptr;
    cScopeGuard scopeGuard([&o_mesh, &result, &newMesh]
        {
            if (result)
            {
                EAE6320_ASSERT(newMesh != nullptr);
                o_mesh = newMesh;
            }
            else
            {
                if (newMesh)
                {
                    newMesh->DecrementReferenceCount();
                    newMesh = nullptr;
                }
                o_mesh = nullptr;
            }
        });

    //Allocate a new Mesh
    {
        newMesh = new cMesh(indexCount);
        if (!newMesh)
        {
            result = Results::OutOfMemory;
            EAE6320_ASSERTF(false, "Couldn't allocate memory for the mesh");
            Logging::OutputError("Failed to allocate memory for the mesh");
            return result;
        }
    }

    //Initialize the geometry
    if (!(result = newMesh->InitializeGeometry(vertexData, vertexCount, indexData)))
    {
        EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
        return result;
    }
    
    if (vertexData)
    {
        delete[] vertexData;
    }

    if (indexData)
    {
        delete[] indexData;
    }
    return result;
}

eae6320::Graphics::cMesh::~cMesh()
{
    EAE6320_ASSERT(m_referenceCount == 0);
    const auto result = CleanUp();
    EAE6320_ASSERT(result);
}

namespace
{
    eae6320::cResult LoadAsset(const char* const i_path, uint16_t*& i_index, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, unsigned int& i_vertexCount, unsigned int& i_indexCount)
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
                eae6320::Logging::OutputError("Failed to create a new Lua state");
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
                eae6320::Logging::OutputError(lua_tostring(luaState, -1));
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
                        eae6320::Logging::OutputError("Asset files must return a table");
                        lua_pop(luaState, 1);
                        return result;
                    }
                }
                else
                {
                    result = eae6320::Results::InvalidFile;
                    eae6320::Logging::OutputError("Asset files must return a single table (instead of %d values)", returnedValueCount);
                    lua_pop(luaState, returnedValueCount);
                    return result;
                }
            }
            else
            {
                result = eae6320::Results::InvalidFile;
                eae6320::Logging::OutputError(lua_tostring(luaState, -1));
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
            eae6320::Logging::OutputError("Failed to load the vertex values from the lua file");
            return result;
        }
        
        if (!(result = LoadTableValues_index(*luaState, i_index, i_indexCount)))
        {
            result = eae6320::Results::InvalidFile;
            eae6320::Logging::OutputError("Failed to load the index values from the lua file");
            return result;
        }

        return result;
    }

    eae6320::cResult LoadTableValues_vertex(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, unsigned int& i_vertexCount)
    {
        auto result = eae6320::Results::Success;
        constexpr auto* const key = "vertex";
        lua_pushstring(&io_luaState, key);
        lua_gettable(&io_luaState, -2);
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
            eae6320::Logging::OutputError("The value must be a table");
            return result;
        }

        return result;
    }

    eae6320::cResult LoadTableValues_index(lua_State& io_luaState, uint16_t*& i_index, unsigned int& i_indexCount)
    {
        auto result = eae6320::Results::Success;
        constexpr auto* const key = "index";
        lua_pushstring(&io_luaState, key);
        lua_gettable(&io_luaState, -2);
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
            eae6320::Logging::OutputError("The value must be a table");
            return result;
        }

        return result;
    }
    
    eae6320::cResult LoadVertexValues(lua_State& io_luaState, eae6320::Graphics::VertexFormats::sVertex_mesh*& i_vertex, unsigned int& i_vertexCount)
    {
        auto result = eae6320::Results::Success;
        float x = 0.0, y = 0.0, z = 0.0;
        const auto arrayLength = static_cast<unsigned int>(luaL_len(&io_luaState, -1));
        i_vertex = new eae6320::Graphics::VertexFormats::sVertex_mesh[arrayLength];
        if ( arrayLength > 0 )
        {
            i_vertexCount = static_cast<unsigned int>(arrayLength);
            for (unsigned int i = 1; i <= arrayLength; ++i)
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
                    eae6320::Logging::OutputError("No value was found %s for in asset table", keyX);
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
                    eae6320::Logging::OutputError("No value was found %s for in asset table", keyY);
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
                    eae6320::Logging::OutputError("No value was found %s for in asset table", keyZ);
                    return result;
                }

                {
                    const auto value = lua_tonumber(&io_luaState, -1);
                    z = static_cast<float>(value);
                }
                i_vertex[i-1] = { x, y, z };
            }
        }
        else
        {
            result = eae6320::Results::InvalidFile;
            eae6320::Logging::OutputError("The asset table doesn't have any ordered values");
            return result;
        }

        return result;
    }

    eae6320::cResult LoadIndexValues(lua_State& io_luaState, uint16_t*& i_index, unsigned int& i_indexCount)
    {
        auto result = eae6320::Results::Success;
        const auto arrayLength = luaL_len(&io_luaState, -1);
        constexpr auto triangleVertexCount = 3;
        if (arrayLength > 0)
        {
            i_indexCount = static_cast<unsigned int>(arrayLength * triangleVertexCount);
            i_index = new uint16_t[i_indexCount];
            for (int i = 1; i <= arrayLength; ++i)
            {
                lua_pushinteger(&io_luaState, i);
                lua_gettable(&io_luaState, -2);

                eae6320::cScopeGuard scopeGuard_popValue([&io_luaState]
                    {
                        lua_pop(&io_luaState, 1);
                    });
                constexpr auto* const key = "vertices";
                lua_pushstring(&io_luaState, key);
                {
                    constexpr int currentIndexOfTable = -2;
                    lua_gettable(&io_luaState, currentIndexOfTable);
                    const auto currentArrayLength = luaL_len(&io_luaState, -1);
                    eae6320::cScopeGuard scopeGuard_popValue([&io_luaState]
                        {
                            lua_pop(&io_luaState, 1);
                        });
                    for (int j = 1; j <= currentArrayLength; ++j)
                    {
                        lua_pushinteger(&io_luaState, j);
                        lua_gettable(&io_luaState, -2);
                        eae6320::cScopeGuard scopeGuard_popValue([&io_luaState]
                            {
                                lua_pop(&io_luaState, 1);
                            });
                        const auto value = lua_tonumber(&io_luaState, -1);
                        i_index[(j-1) + ((i-1)*currentArrayLength)] = static_cast<uint16_t>(value);
                    }
                }
            }
        }

        return result;
    }
}