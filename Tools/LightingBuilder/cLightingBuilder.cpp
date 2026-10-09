// Includes
//=========

#include "cLightingBuilder.h"

#include <cstring>
#include <Engine/Platform/Platform.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <initializer_list>
#include <string>
#include <Tools/AssetBuildLibrary/Functions.h>
#include <vector>

// Static Data
//============

namespace
{
	// The valid range of each kind of value.
	// These limits aren't physical laws; they exist to catch typos
	// (e.g. an intensity of 500 instead of 5.0) at build time.
	struct sRange
	{
		double min, max;
	};
	constexpr sRange s_range_color{ 0.0, 1.0 };	// Brightness belongs in the intensity, not the color
	constexpr sRange s_range_intensity{ 0.0, 100.0 };
	constexpr sRange s_range_specularIntensity{ 0.0, 10.0 };
	constexpr sRange s_range_shininess{ 1.0, 2048.0 };	// Below 1 the highlight covers the whole lit side of an object
	constexpr sRange s_range_pointLightRange{ 0.01, 1000.0 };
	constexpr sRange s_range_coordinate{ -100000.0, 100000.0 };
}

// Helper Declarations
//====================

namespace
{
	// Each of these expects the table being read to be at the top of the Lua stack
	// and leaves the stack exactly as it found it.
	// i_tableName is only used to make error messages specific (e.g. "pointLights[2].range").

	eae6320::cResult ValidateKeys( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const std::initializer_list<const char*> i_validKeys );
	eae6320::cResult ReadNumber( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, const sRange& i_range, float& io_value );
	eae6320::cResult ReadVector3( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, const sRange& i_range, float& io_x, float& io_y, float& io_z );
	eae6320::cResult ReadColor( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, eae6320::Lighting::sColor& io_color );
	eae6320::cResult ReadVector( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, eae6320::Math::sVector& io_vector );
}

// Inherited Implementation
//=========================

// Build
//------

eae6320::cResult eae6320::Assets::cLightingBuilder::Build( const std::vector<std::string>& )
{
	auto result = Results::Success;

	// Start with the defaults and overwrite whatever the file specifies
	Lighting::sLightingData lightingData;
	if ( !( result = LoadAsset( m_path_source, lightingData ) ) )
	{
		return result;
	}

	// The shaders need a unit-length direction.
	// Normalizing here means the file can use any convenient length
	// (e.g. { 1, -2, -1 } instead of working out { 0.408, -0.816, -0.408 } by hand).
	// (LoadDirectionalLight() already made sure that the length isn't zero.)
	lightingData.directional.direction.Normalize();

	// Write the binary file: the header followed by the data
	{
		const Lighting::sFileHeader header;
		std::vector<uint8_t> fileData( sizeof( header ) + sizeof( lightingData ) );
		memcpy( fileData.data(), &header, sizeof( header ) );
		memcpy( fileData.data() + sizeof( header ), &lightingData, sizeof( lightingData ) );

		std::string errorMessage;
		if ( !( result = Platform::WriteBinaryFile( m_path_target, fileData.data(), fileData.size(), &errorMessage ) ) )
		{
			OutputErrorMessageWithFileInfo( m_path_target, errorMessage.c_str() );
			return result;
		}
	}

	return result;
}

// Implementation
//===============

eae6320::cResult eae6320::Assets::cLightingBuilder::LoadAsset( const char* const i_path, Lighting::sLightingData& o_lightingData )
{
	auto result = Results::Success;

	// Create a new Lua state
	lua_State* luaState = nullptr;
	cScopeGuard scopeGuard_onExit( [&luaState]
		{
			if ( luaState )
			{
				lua_close( luaState );
				luaState = nullptr;
			}
		} );
	{
		luaState = luaL_newstate();
		if ( !luaState )
		{
			result = Results::OutOfMemory;
			OutputErrorMessageWithFileInfo( i_path, "Failed to create a new Lua state" );
			return result;
		}
	}

	// Load and execute the file, which must return a single table
	{
		const auto luaResult = luaL_loadfile( luaState, i_path );
		if ( luaResult != LUA_OK )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( i_path, lua_tostring( luaState, -1 ) );
			lua_pop( luaState, 1 );
			return result;
		}
	}
	{
		constexpr int argumentCount = 0;
		constexpr int returnValueCount = LUA_MULTRET;
		constexpr int noMessageHandler = 0;
		const auto luaResult = lua_pcall( luaState, argumentCount, returnValueCount, noMessageHandler );
		if ( luaResult != LUA_OK )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( i_path, lua_tostring( luaState, -1 ) );
			lua_pop( luaState, 1 );
			return result;
		}
		const auto returnedValueCount = lua_gettop( luaState );
		if ( ( returnedValueCount != 1 ) || !lua_istable( luaState, -1 ) )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( i_path, "Lighting files must return a single table" );
			lua_pop( luaState, returnedValueCount );
			return result;
		}
	}
	cScopeGuard scopeGuard_popAssetTable( [luaState]
		{
			lua_pop( luaState, 1 );
		} );

	if ( !( result = ValidateKeys( *luaState, i_path, "the lighting table", { "ambient", "directional", "specular", "pointLights" } ) ) )
	{
		return result;
	}

	// Each section is optional
	const auto LoadSection = [luaState, i_path]( const char* const i_key, auto&& i_loadFunction ) -> cResult
	{
		lua_getfield( luaState, -1, i_key );
		cScopeGuard scopeGuard_popSection( [luaState]
			{
				lua_pop( luaState, 1 );
			} );
		if ( lua_isnil( luaState, -1 ) )
		{
			return Results::Success;
		}
		if ( !lua_istable( luaState, -1 ) )
		{
			OutputErrorMessageWithFileInfo( i_path, "%s must be a table (instead of a %s)", i_key, luaL_typename( luaState, -1 ) );
			return Results::InvalidFile;
		}
		return i_loadFunction();
	};
	if ( !( result = LoadSection( "ambient", [&] { return LoadAmbientLight( *luaState, o_lightingData.ambient ); } ) ) )
	{
		return result;
	}
	if ( !( result = LoadSection( "directional", [&] { return LoadDirectionalLight( *luaState, o_lightingData.directional ); } ) ) )
	{
		return result;
	}
	if ( !( result = LoadSection( "specular", [&] { return LoadSpecular( *luaState, o_lightingData.specular ); } ) ) )
	{
		return result;
	}
	if ( !( result = LoadSection( "pointLights", [&] { return LoadPointLights( *luaState, o_lightingData ); } ) ) )
	{
		return result;
	}

	return result;
}

eae6320::cResult eae6320::Assets::cLightingBuilder::LoadAmbientLight( lua_State& io_luaState, Lighting::sAmbientLight& o_ambientLight )
{
	auto result = Results::Success;
	constexpr auto* const tableName = "ambient";
	if ( !( result = ValidateKeys( io_luaState, m_path_source, tableName, { "skyColor", "groundColor", "intensity" } ) )
		|| !( result = ReadColor( io_luaState, m_path_source, tableName, "skyColor", o_ambientLight.skyColor ) )
		|| !( result = ReadColor( io_luaState, m_path_source, tableName, "groundColor", o_ambientLight.groundColor ) )
		|| !( result = ReadNumber( io_luaState, m_path_source, tableName, "intensity", s_range_intensity, o_ambientLight.intensity ) ) )
	{
		return result;
	}
	return result;
}

eae6320::cResult eae6320::Assets::cLightingBuilder::LoadDirectionalLight( lua_State& io_luaState, Lighting::sDirectionalLight& o_directionalLight )
{
	auto result = Results::Success;
	constexpr auto* const tableName = "directional";
	if ( !( result = ValidateKeys( io_luaState, m_path_source, tableName, { "direction", "color", "intensity" } ) )
		|| !( result = ReadVector( io_luaState, m_path_source, tableName, "direction", o_directionalLight.direction ) )
		|| !( result = ReadColor( io_luaState, m_path_source, tableName, "color", o_directionalLight.color ) )
		|| !( result = ReadNumber( io_luaState, m_path_source, tableName, "intensity", s_range_intensity, o_directionalLight.intensity ) ) )
	{
		return result;
	}
	// A zero-length direction can't be normalized and doesn't point anywhere
	if ( !( o_directionalLight.direction.GetLength() > 1.0e-6f ) )
	{
		result = Results::InvalidFile;
		OutputErrorMessageWithFileInfo( m_path_source, "directional.direction can't be { 0, 0, 0 }" );
		return result;
	}
	return result;
}

eae6320::cResult eae6320::Assets::cLightingBuilder::LoadSpecular( lua_State& io_luaState, Lighting::sSpecular& o_specular )
{
	auto result = Results::Success;
	constexpr auto* const tableName = "specular";
	if ( !( result = ValidateKeys( io_luaState, m_path_source, tableName, { "intensity", "shininess" } ) )
		|| !( result = ReadNumber( io_luaState, m_path_source, tableName, "intensity", s_range_specularIntensity, o_specular.intensity ) )
		|| !( result = ReadNumber( io_luaState, m_path_source, tableName, "shininess", s_range_shininess, o_specular.shininess ) ) )
	{
		return result;
	}
	return result;
}

eae6320::cResult eae6320::Assets::cLightingBuilder::LoadPointLights( lua_State& io_luaState, Lighting::sLightingData& o_lightingData )
{
	auto result = Results::Success;

	// pointLights is an array: { { ... }, { ... } }
	const auto lightCount = luaL_len( &io_luaState, -1 );
	if ( lightCount > static_cast<lua_Integer>( Lighting::MaxPointLightCount ) )
	{
		result = Results::InvalidFile;
		OutputErrorMessageWithFileInfo( m_path_source, "There are %d point lights but the maximum is %u"
			" (the shaders use a fixed-size array; see Lighting::MaxPointLightCount)",
			static_cast<int>( lightCount ), Lighting::MaxPointLightCount );
		return result;
	}
	// Anything other than the keys 1 through lightCount (e.g. pointLights = { light1 = { ... } })
	// would be ignored by luaL_len(), so it is treated as an error
	{
		lua_pushnil( &io_luaState );
		while ( lua_next( &io_luaState, -2 ) != 0 )
		{
			const auto isValidKey = lua_isinteger( &io_luaState, -2 )
				&& ( lua_tointeger( &io_luaState, -2 ) >= 1 ) && ( lua_tointeger( &io_luaState, -2 ) <= lightCount );
			lua_pop( &io_luaState, 1 );
			if ( !isValidKey )
			{
				lua_pop( &io_luaState, 1 );
				result = Results::InvalidFile;
				OutputErrorMessageWithFileInfo( m_path_source, "pointLights must be a list of lights: { { position = ... }, { position = ... } }" );
				return result;
			}
		}
	}

	o_lightingData.pointLightCount = static_cast<uint32_t>( lightCount );
	for ( lua_Integer i = 1; i <= lightCount; ++i )
	{
		lua_geti( &io_luaState, -1, i );
		cScopeGuard scopeGuard_popLight( [&io_luaState]
			{
				lua_pop( &io_luaState, 1 );
			} );
		if ( !lua_istable( &io_luaState, -1 ) )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( m_path_source, "pointLights[%d] must be a table", static_cast<int>( i ) );
			return result;
		}
		if ( !( result = LoadPointLight( io_luaState, static_cast<unsigned int>( i ), o_lightingData.pointLights[i - 1] ) ) )
		{
			return result;
		}
	}

	return result;
}

eae6320::cResult eae6320::Assets::cLightingBuilder::LoadPointLight( lua_State& io_luaState, const unsigned int i_lightNumber,
	Lighting::sPointLight& o_pointLight )
{
	auto result = Results::Success;
	const auto tableName = "pointLights[" + std::to_string( i_lightNumber ) + "]";
	// Unlike the other lights a point light has no sensible default position,
	// so the position is required
	{
		lua_getfield( &io_luaState, -1, "position" );
		const auto hasPosition = !lua_isnil( &io_luaState, -1 );
		lua_pop( &io_luaState, 1 );
		if ( !hasPosition )
		{
			result = Results::InvalidFile;
			OutputErrorMessageWithFileInfo( m_path_source, "%s must have a position", tableName.c_str() );
			return result;
		}
	}
	if ( !( result = ValidateKeys( io_luaState, m_path_source, tableName.c_str(), { "position", "color", "intensity", "range" } ) )
		|| !( result = ReadVector( io_luaState, m_path_source, tableName.c_str(), "position", o_pointLight.position ) )
		|| !( result = ReadColor( io_luaState, m_path_source, tableName.c_str(), "color", o_pointLight.color ) )
		|| !( result = ReadNumber( io_luaState, m_path_source, tableName.c_str(), "intensity", s_range_intensity, o_pointLight.intensity ) )
		|| !( result = ReadNumber( io_luaState, m_path_source, tableName.c_str(), "range", s_range_pointLightRange, o_pointLight.range ) ) )
	{
		return result;
	}
	return result;
}

// Helper Definitions
//===================

namespace
{
	eae6320::cResult ValidateKeys( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const std::initializer_list<const char*> i_validKeys )
	{
		// lua_next() visits every key/value pair in the table
		lua_pushnil( &io_luaState );
		while ( lua_next( &io_luaState, -2 ) != 0 )
		{
			// The key is at -2 and the value is at -1
			bool isValidKey = false;
			std::string keyDescription;
			// (lua_tostring() must not be called on a non-string key because it would convert the key in place and confuse lua_next())
			if ( lua_type( &io_luaState, -2 ) == LUA_TSTRING )
			{
				const auto* const key = lua_tostring( &io_luaState, -2 );
				keyDescription = std::string( "\"" ) + key + "\"";
				for ( const auto* const validKey : i_validKeys )
				{
					if ( strcmp( key, validKey ) == 0 )
					{
						isValidKey = true;
						break;
					}
				}
			}
			else
			{
				keyDescription = std::string( "a key of type " ) + luaL_typename( &io_luaState, -2 );
			}
			// Pop the value (the key stays for the next call to lua_next())
			lua_pop( &io_luaState, 1 );
			if ( !isValidKey )
			{
				// Pop the key too because iteration is stopping early
				lua_pop( &io_luaState, 1 );
				std::string validKeys;
				for ( const auto* const validKey : i_validKeys )
				{
					validKeys += ( validKeys.empty() ? "" : ", " ) + std::string( validKey );
				}
				eae6320::Assets::OutputErrorMessageWithFileInfo( i_path, "%s has %s, which isn't recognized (the valid keys are: %s)",
					i_tableName, keyDescription.c_str(), validKeys.c_str() );
				return eae6320::Results::InvalidFile;
			}
		}
		return eae6320::Results::Success;
	}

	eae6320::cResult ReadNumber( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, const sRange& i_range, float& io_value )
	{
		lua_getfield( &io_luaState, -1, i_key );
		eae6320::cScopeGuard scopeGuard_popValue( [&io_luaState]
			{
				lua_pop( &io_luaState, 1 );
			} );
		if ( lua_isnil( &io_luaState, -1 ) )
		{
			// Keep the default
			return eae6320::Results::Success;
		}
		// (lua_type() is used instead of lua_isnumber() because lua_isnumber() also accepts strings like "5")
		if ( lua_type( &io_luaState, -1 ) != LUA_TNUMBER )
		{
			eae6320::Assets::OutputErrorMessageWithFileInfo( i_path, "%s.%s must be a number (instead of a %s)",
				i_tableName, i_key, luaL_typename( &io_luaState, -1 ) );
			return eae6320::Results::InvalidFile;
		}
		const auto value = lua_tonumber( &io_luaState, -1 );
		// (This is written so that NaN, which fails every comparison, is also rejected)
		if ( !( ( value >= i_range.min ) && ( value <= i_range.max ) ) )
		{
			eae6320::Assets::OutputErrorMessageWithFileInfo( i_path, "%s.%s is %g but must be between %g and %g",
				i_tableName, i_key, value, i_range.min, i_range.max );
			return eae6320::Results::InvalidFile;
		}
		io_value = static_cast<float>( value );
		return eae6320::Results::Success;
	}

	eae6320::cResult ReadVector3( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, const sRange& i_range, float& io_x, float& io_y, float& io_z )
	{
		lua_getfield( &io_luaState, -1, i_key );
		eae6320::cScopeGuard scopeGuard_popValue( [&io_luaState]
			{
				lua_pop( &io_luaState, 1 );
			} );
		if ( lua_isnil( &io_luaState, -1 ) )
		{
			// Keep the default
			return eae6320::Results::Success;
		}
		if ( !lua_istable( &io_luaState, -1 ) || ( luaL_len( &io_luaState, -1 ) != 3 ) )
		{
			eae6320::Assets::OutputErrorMessageWithFileInfo( i_path, "%s.%s must be a list of three numbers, e.g. { 1.0, 0.5, 0.0 }",
				i_tableName, i_key );
			return eae6320::Results::InvalidFile;
		}
		float* const values[] = { &io_x, &io_y, &io_z };
		for ( lua_Integer i = 1; i <= 3; ++i )
		{
			lua_geti( &io_luaState, -1, i );
			const auto isNumber = lua_type( &io_luaState, -1 ) == LUA_TNUMBER;
			const auto value = isNumber ? lua_tonumber( &io_luaState, -1 ) : 0.0;
			lua_pop( &io_luaState, 1 );
			if ( !isNumber )
			{
				eae6320::Assets::OutputErrorMessageWithFileInfo( i_path, "%s.%s[%d] must be a number", i_tableName, i_key, static_cast<int>( i ) );
				return eae6320::Results::InvalidFile;
			}
			if ( !( ( value >= i_range.min ) && ( value <= i_range.max ) ) )
			{
				eae6320::Assets::OutputErrorMessageWithFileInfo( i_path, "%s.%s[%d] is %g but must be between %g and %g",
					i_tableName, i_key, static_cast<int>( i ), value, i_range.min, i_range.max );
				return eae6320::Results::InvalidFile;
			}
			*values[i - 1] = static_cast<float>( value );
		}
		return eae6320::Results::Success;
	}

	eae6320::cResult ReadColor( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, eae6320::Lighting::sColor& io_color )
	{
		return ReadVector3( io_luaState, i_path, i_tableName, i_key, s_range_color, io_color.r, io_color.g, io_color.b );
	}

	eae6320::cResult ReadVector( lua_State& io_luaState, const char* const i_path, const char* const i_tableName,
		const char* const i_key, eae6320::Math::sVector& io_vector )
	{
		return ReadVector3( io_luaState, i_path, i_tableName, i_key, s_range_coordinate, io_vector.x, io_vector.y, io_vector.z );
	}
}
