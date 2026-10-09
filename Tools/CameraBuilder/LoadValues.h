#pragma once

#include <Engine/Results/Results.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <External/Lua/Includes.h>

#define _SCOPE_POP_TOP_SUFFIX(luaState, name) cScopeGuard scopeGuardPopTop_ ## name ## ([&luaState] { lua_pop(&luaState, 1); });
#define SCOPE_POP_TOP_SUFFIX(luaState, name) _SCOPE_POP_TOP_SUFFIX(luaState, name)
#define SCOPE_POP_TOP(luaState) SCOPE_POP_TOP_SUFFIX(luaState, __LINE__)

#define LUA_GET_FROM_DICT(luaState, key) \
	lua_pushstring(&io_luaState, key); \
	lua_gettable(&io_luaState, -2); \
	SCOPE_POP_TOP(io_luaState);

#define LUA_GET_FROM_ARRAY(luaState, index) \
	lua_pushinteger(&io_luaState, index); \
	lua_gettable(&io_luaState, -2); \
	SCOPE_POP_TOP(io_luaState); \

namespace eae6320::Math
{
	struct sRectangle;
	struct sVector2D;
	struct sVector;
}

namespace eae6320::Assets
{
	cResult LoadBool(lua_State& io_luaState, const char* i_key, bool& o_bool);
	cResult LoadBoolAcceptNil(lua_State& io_luaState, const char* i_key, bool& o_bool, bool& o_wasBoolLoaded);

	cResult LoadFloat(lua_State& io_luaState, const char* i_key, float& o_float);
	cResult LoadFloatAcceptNil(lua_State& io_luaState, const char* i_key, float& o_float, bool& o_wasFloatLoaded);

	cResult LoadStr(lua_State& io_luaState, const char* i_key, const char*& o_string);
	cResult LoadStrAcceptNil(lua_State& io_luaState, const char* i_key, const char*& o_string, bool& o_wasStringLoaded);

	cResult LoadVector2D(lua_State& io_luaState, const char* i_key, Math::sVector2D& o_vector2d);
	cResult LoadVector2DAcceptNil(lua_State& io_luaState, const char* i_key, Math::sVector2D& o_vector2d, bool& o_wasVector2DLoaded);

	cResult LoadVector(lua_State& io_luaState, const char* i_key, Math::sVector& o_vector);
	cResult LoadVectorAcceptNil(lua_State& io_luaState, const char* i_key, Math::sVector& o_vector, bool& o_wasVectorLoaded);

	cResult LoadRectangle(lua_State& io_luaState, const char* i_key, Math::sRectangle& o_rectangle);
	cResult LoadRectangleAcceptNil(lua_State& io_luaState, const char* i_key, Math::sRectangle& o_rectangle, bool& o_wasRectangleLoaded);
}