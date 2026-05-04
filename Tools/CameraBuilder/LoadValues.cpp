#include "LoadValues.h"
#include <Engine/Logging/Logging.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/sRectangle.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/sVector2D.h>

namespace eae6320::Assets
{
	cResult _LoadBool(lua_State& io_luaState, const char* i_key, bool i_acceptNil, bool& o_bool, bool& o_wasBoolLoaded);
	cResult _LoadFloat(lua_State& io_luaState, const char* i_key, bool i_acceptNil, float& o_number, bool& o_wasFloatLoaded);
	cResult _LoadStr(lua_State& io_luaState, const char* i_key, bool i_acceptNil, const char*& o_string, bool& o_wasStringLoaded);
	cResult _LoadVector2D(lua_State& io_luaState, const char* i_key, bool i_acceptNil, Math::sVector2D& o_vector2d, bool& o_wasVector2DLoaded);
	cResult _LoadVector(lua_State& io_luaState, const char* i_key, bool i_acceptNil, Math::sVector& o_vector, bool& o_wasVectorLoaded);
	cResult _LoadRectangle(lua_State& io_luaState, const char* i_key, bool i_acceptNil, Math::sRectangle& o_rectangle, bool& o_wasRectangleLoaded);

	cResult _LoadBool(lua_State& io_luaState, const char* i_key, bool i_acceptNil, bool& o_bool, bool& o_wasBoolLoaded)
	{
		// top: { [your i_key] = true }

		LUA_GET_FROM_DICT(io_luaState, i_key);
		// top: true

		if (!lua_isboolean(&io_luaState, -1))
		{
			EAE6320_ASSERTF(false, "Expected boolean from key %s, received %s", i_key, lua_typename(&io_luaState, -1));
			return Results::InvalidFile;
		}

		o_bool = static_cast<bool>(lua_toboolean(&io_luaState, -1));

		return Results::Success;
	}

	cResult _LoadFloat(lua_State& io_luaState, const char* i_key, bool i_acceptNil, float& o_number, bool& o_wasFloatLoaded)
	{
		// top: { [your i_key] = ... }

		LUA_GET_FROM_DICT(io_luaState, i_key);
		// top: 1.0

		if (!lua_isnumber(&io_luaState, -1))
		{
			EAE6320_ASSERTF(false, "Expected number from key %s, received %s", i_key, lua_typename(&io_luaState, -1));
			return Results::InvalidFile;
		}

		o_number = static_cast<float>(lua_tonumber(&io_luaState, -1));

		return Results::Success;
	}

	cResult _LoadStr(lua_State& io_luaState, const char* i_key, bool i_acceptNil, const char*& o_string, bool& o_wasStringLoaded)
	{
		// top: { [your i_key] = "string" }

		LUA_GET_FROM_DICT(io_luaState, i_key);
		// top: "string"

		if (!lua_isstring(&io_luaState, -1))
		{
			EAE6320_ASSERTF(false, "Expected string from key %s, received %s", i_key, lua_typename(&io_luaState, -1));
			return Results::InvalidFile;
		}

		o_string = lua_tostring(&io_luaState, -1);

		return Results::Success;
	}

	cResult _LoadVector2D(lua_State& io_luaState, const char* i_key, bool i_acceptNil, Math::sVector2D& o_vector2d, bool& o_wasVector2DLoaded)
	{
		// top: { [your i_key] = { ... } }

		LUA_GET_FROM_DICT(io_luaState, i_key);
		// top: { 1.0, 2.0 }

		if (!lua_istable(&io_luaState, -1))
		{
			EAE6320_ASSERTF(false, "Expected array from key \"%s\", received %s", i_key, lua_typename(&io_luaState, -1));
			return Results::InvalidFile;
		}
		const auto numIndices = luaL_len(&io_luaState, -1);
		if (numIndices != 2)
		{
			EAE6320_ASSERTF(false, "Expected 2 elements from array \"%s\", received %d", i_key, numIndices);
			return Results::InvalidFile;
		}

		float vertexData[2] = { 0.f };

		for (int i = 1; i <= numIndices; i++)
		{
			LUA_GET_FROM_ARRAY(io_luaState, i);
			// top: 1.0

			if (!lua_isnumber(&io_luaState, -1))
			{
				EAE6320_ASSERTF(false, "Expected number from %s[%d], received %s", i_key, i, lua_typename(&io_luaState, -1));
				return Results::InvalidFile;
			}

			vertexData[i - 1] = static_cast<float>(lua_tonumber(&io_luaState, -1));
		}

		o_vector2d = Math::sVector2D(vertexData[0], vertexData[1]);
		return Results::Success;
	}

	cResult _LoadVector(lua_State& io_luaState, const char* i_key, bool i_acceptNil, Math::sVector& o_vector, bool& o_wasVectorLoaded)
	{
		// top: { [your i_key] = { ... } }

		LUA_GET_FROM_DICT(io_luaState, i_key);
		// top: { 1.0, 2.0, 3.0 }

		if (!lua_istable(&io_luaState, -1))
		{
			EAE6320_ASSERTF(false, "Expected array from key \"%s\", received %s", i_key, lua_typename(&io_luaState, -1));
			return Results::InvalidFile;
		}
		const auto numIndices = luaL_len(&io_luaState, -1);
		if (numIndices != 3)
		{
			EAE6320_ASSERTF(false, "Expected 3 elements from array \"%s\", received %d", i_key, numIndices);
			return Results::InvalidFile;
		}

		float vertexData[3] = { 0.f };

		for (int i = 1; i <= numIndices; i++)
		{
			LUA_GET_FROM_ARRAY(io_luaState, i);
			// top: 1.0

			if (!lua_isnumber(&io_luaState, -1))
			{
				EAE6320_ASSERTF(false, "Expected number from %s[%d], received %s", i_key, i, lua_typename(&io_luaState, -1));
				return Results::InvalidFile;
			}

			vertexData[i - 1] = static_cast<float>(lua_tonumber(&io_luaState, -1));
		}

		o_vector = Math::sVector(vertexData[0], vertexData[1], vertexData[2]);
		return Results::Success;
	}

	cResult _LoadRectangle(lua_State& io_luaState, const char* i_key, bool i_acceptNil, Math::sRectangle& o_rectangle, bool& o_wasRectangleLoaded)
	{
		// top: { [your i_key] = { ... } }

		LUA_GET_FROM_DICT(io_luaState, i_key);
		// top: { left = 0.0, ... }

		if (!lua_istable(&io_luaState, -1))
		{
			EAE6320_ASSERTF(false, "Expected dict from key \"%s\", received %s", i_key, lua_typename(&io_luaState, -1));
			return Results::InvalidFile;
		}

		constexpr const char* left = "left";
		constexpr const char* right = "right";
		constexpr const char* bottom = "bottom";
		constexpr const char* top = "top";

		cResult result = Results::Success;

		if (!LoadFloat(io_luaState, left, o_rectangle.left))
		{
			Logging::OutputError("Failed to load rectangle with key %s (key %s failed)", i_key, left);
			result = Results::InvalidFile;
		}
		if (!LoadFloat(io_luaState, right, o_rectangle.right))
		{
			Logging::OutputError("Failed to load rectangle with key %s (key %s failed)", i_key, right);
			result = Results::InvalidFile;
		}
		if (!LoadFloat(io_luaState, bottom, o_rectangle.bottom))
		{
			Logging::OutputError("Failed to load rectangle with key %s (key %s failed)", i_key, bottom);
			result = Results::InvalidFile;
		}
		if (!LoadFloat(io_luaState, top, o_rectangle.top))
		{
			Logging::OutputError("Failed to load rectangle with key %s (key %s failed)", i_key, top);
			result = Results::InvalidFile;
		}

		return result;
	}

	cResult LoadBool(lua_State& io_luaState, const char* i_key, bool& o_bool)
	{
		bool _ = false;
		return _LoadBool(io_luaState, i_key, false, o_bool, _);
	}

	cResult LoadBoolAcceptNil(lua_State& io_luaState, const char* i_key, bool& o_bool, bool& o_wasBoolLoaded)
	{
		return _LoadBool(io_luaState, i_key, true, o_bool, o_wasBoolLoaded);
	}

	cResult LoadFloat(lua_State& io_luaState, const char* i_key, float& o_float)
	{
		bool _ = false;
		return _LoadFloat(io_luaState, i_key, false, o_float, _);
	}

	cResult LoadFloatAcceptNil(lua_State& io_luaState, const char* i_key, float& o_float, bool& o_wasFloatLoaded)
	{
		return _LoadFloat(io_luaState, i_key, true, o_float, o_wasFloatLoaded);
	}

	cResult LoadStr(lua_State& io_luaState, const char* i_key, const char*& o_string)
	{
		bool _ = false;
		return _LoadStr(io_luaState, i_key, false, o_string, _);
	}

	cResult LoadStrAcceptNil(lua_State& io_luaState, const char* i_key, const char*& o_string, bool& o_wasStringLoaded)
	{
		return _LoadStr(io_luaState, i_key, true, o_string, o_wasStringLoaded);
	}

	cResult LoadVector2D(lua_State& io_luaState, const char* i_key, Math::sVector2D& o_vector2d)
	{
		bool _ = false;
		return _LoadVector2D(io_luaState, i_key, false, o_vector2d, _);
	}

	cResult LoadVector2DAcceptNil(lua_State& io_luaState, const char* i_key, Math::sVector2D& o_vector2d, bool& o_wasVector2DLoaded)
	{
		return _LoadVector2D(io_luaState, i_key, true, o_vector2d, o_wasVector2DLoaded);
	}

	cResult LoadVector(lua_State& io_luaState, const char* i_key, Math::sVector& o_vector)
	{
		bool _ = false;
		return _LoadVector(io_luaState, i_key, false, o_vector, _);
	}

	cResult LoadVectorAcceptNil(lua_State& io_luaState, const char* i_key, Math::sVector& o_vector, bool& o_wasVectorLoaded)
	{
		return _LoadVector(io_luaState, i_key, true, o_vector, o_wasVectorLoaded);
	}

	cResult LoadRectangle(lua_State& io_luaState, const char* i_key, Math::sRectangle& o_rectangle)
	{
		bool _ = false;
		return _LoadRectangle(io_luaState, i_key, false, o_rectangle, _);
	}

	cResult LoadRectangleAcceptNil(lua_State& io_luaState, const char* i_key, Math::sRectangle& o_rectangle, bool& o_wasRectangleLoaded)
	{
		return _LoadRectangle(io_luaState, i_key, true, o_rectangle, o_wasRectangleLoaded);
	}
}