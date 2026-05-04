// Includes
//=========

#include "cCameraBuilder.h"

#include <string>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <utility>
#include <fstream>
#include <Engine/Asserts/Asserts.h>
#include <Engine/CameraControls/Position/iPositionStrategy.h>
#include <Engine/CameraControls/Orientation/iOrientationStrategy.h>
#include <Engine/CameraControls/cTrackingCamera.h>
#include <Engine/CameraControls/Serialization.h>
#include <Engine/Logging/Logging.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <External/Lua/Includes.h>
#include <Tools/AssetBuildLibrary/Functions.h>
#include "LoadValues.h"
#include "LoadOrientationStragies.h"
#include "LoadPositionStrategies.h"

// Inherited Implementation
//=========================

// Build
//------

namespace eae6320::Assets
{
	struct sSerializedCamera
	{
		std::string serializedCamera;
		size_t serializedCameraSize = 0;
		std::string serializedPositionStrategy;
		size_t serializedPositionStrategySize = 0;
		std::string serializedOrientationStrategy;
		size_t serializedOrientationStrategySize = 0;
		int16_t positionStrategyClassId = 0;
		int16_t orientationStrategyClassId = 0;
	};

	cResult OutputCameraToFile(const sSerializedCamera& i_serializedCamera, const char* i_outfile);
	cResult LoadCameraFile(const char* i_cameraPath, sSerializedCamera& o_serializedCamera);
	cResult LoadTable(lua_State*& o_luaState, const char* i_cameraPath);

	cResult LoadCamera(lua_State& io_luaState, const char* i_filename, sSerializedCamera& o_serializedCamera);
	cResult LoadCameraInfo(lua_State& io_luaState, sSerializedCamera& o_serializedCamera);

	cResult LoadSerializedPositionStrategy(lua_State& io_luaState, sSerializedCamera& o_serializedCamera);

	cResult LoadSerializedOrientationStrategy(lua_State& io_luaState, sSerializedCamera& o_serializedCamera);

	cResult cCameraBuilder::Build(const std::vector<std::string>& i_arguments)
	{
		sSerializedCamera cameraInfo;

		cResult loadResult = LoadCameraFile(m_path_source, cameraInfo);
		if (!loadResult)
		{
			OutputErrorMessageWithFileInfo(m_path_source, "Failed to load camera file %s", m_path_source);
			return loadResult;
		}

		cResult toFileResult = OutputCameraToFile(cameraInfo, m_path_target);
		if (!toFileResult)
		{
			OutputErrorMessageWithFileInfo(m_path_source, "Failed to write binary camera data to output file %s", m_path_target);
			return toFileResult;
		}

		return Results::Success;
	}

	cResult OutputCameraToFile(const sSerializedCamera& i_serializedCamera, const char* i_outfile)
	{
		Serialization::sCameraFileMetadata metadata;
		metadata.positionStrategyClassId = i_serializedCamera.positionStrategyClassId;
		metadata.orientationStrategyClassId = i_serializedCamera.orientationStrategyClassId;
		metadata.cameraDataSize = static_cast<uint32_t>(i_serializedCamera.serializedCameraSize);
		metadata.positionStrategySize = static_cast<uint32_t>(i_serializedCamera.serializedPositionStrategySize);
		metadata.orientationStrategySize = static_cast<uint32_t>(i_serializedCamera.serializedOrientationStrategySize);

		metadata.checksum = Serialization::ComputeChecksum(metadata,
			i_serializedCamera.serializedCamera.data(),
			i_serializedCamera.serializedPositionStrategy.data(),
			i_serializedCamera.serializedOrientationStrategy.data());

		std::ofstream outfile(i_outfile, std::ofstream::binary);
		eae6320::cScopeGuard scopeGuard_onExit([&outfile]
			{
				outfile.close();
			});

		outfile.write(reinterpret_cast<const char*>(&metadata), sizeof(Serialization::sCameraFileMetadata));
		if (outfile.fail())
		{
			OutputErrorMessageWithFileInfo(i_outfile, "Failed to write metadata for binary camera data");
			return Results::Failure;
		}
		outfile.write(reinterpret_cast<const char*>(i_serializedCamera.serializedCamera.data()), i_serializedCamera.serializedCameraSize);
		if (outfile.fail())
		{
			OutputErrorMessageWithFileInfo(i_outfile, "Failed to write camera info data for binary camera data");
			return Results::Failure;
		}
		outfile.write(reinterpret_cast<const char*>(i_serializedCamera.serializedPositionStrategy.data()), i_serializedCamera.serializedPositionStrategySize);
		if (outfile.fail())
		{
			OutputErrorMessageWithFileInfo(i_outfile, "Failed to write camera position strategy data for binary camera data");
			return Results::Failure;
		}
		outfile.write(reinterpret_cast<const char*>(i_serializedCamera.serializedOrientationStrategy.data()), i_serializedCamera.serializedOrientationStrategySize);
		if (outfile.fail())
		{
			OutputErrorMessageWithFileInfo(i_outfile, "Failed to write camera orientation strategy for binary camera data");
			return Results::Failure;
		}

		return Results::Success;
	}

	cResult LoadCameraFile(const char* i_cameraPath, sSerializedCamera& o_serializedCamera)
	{
		auto result = Results::Success;

		lua_State* luaState = nullptr;
		result = LoadTable(luaState, i_cameraPath);
		if (!result)
		{
			Logging::OutputError("Failed to load mesh file at %s", i_cameraPath);
			return result;
		}

		eae6320::cScopeGuard scopeGuard_onExit([&luaState]
			{
				lua_pop(luaState, 1);
				EAE6320_ASSERT(lua_gettop(luaState) == 0);

				lua_close(luaState);
				luaState = nullptr;
			});

		result = LoadCamera(*luaState, i_cameraPath, o_serializedCamera);
		if (!result)
		{
			EAE6320_ASSERTF(false, "Failed to load camera from file %s", i_cameraPath);
			return result;
		}

		return result;
	}

	cResult LoadTable(lua_State*& o_luaState, const char* i_cameraPath)
	{
		auto result = Results::Success;

		eae6320::cScopeGuard scopeGuard_onExit([&o_luaState, &result]
			{
				// Close the Lua state if we weren't successful
				if (o_luaState && !result)
				{
					// If I haven't made any mistakes
					// there shouldn't be anything on the stack
					// regardless of any errors
					EAE6320_ASSERT(lua_gettop(o_luaState) == 0);

					lua_close(o_luaState);
					o_luaState = nullptr;
				}
			});

		o_luaState = luaL_newstate();
		if (!o_luaState)
		{
			Logging::OutputError("Failed to create a new Lua state");
			o_luaState = nullptr;
			result = Results::OutOfMemory;
			return result;
		}

		// Load the asset file as a "chunk",
		// meaning there will be a callable function at the top of the stack
		const auto stackTopBeforeLoad = lua_gettop(o_luaState);
		{
			if (luaL_loadfile(o_luaState, i_cameraPath) != LUA_OK)
			{
				Logging::OutputError(lua_tostring(o_luaState, -1));
				lua_pop(o_luaState, 1);
				result = Results::InvalidFile;
				return result;
			}
		}

		// Execute the "chunk", which should load the asset
		// into a table at the top of the stack
		{
			constexpr int argumentCount = 0;
			constexpr int returnValueCount = LUA_MULTRET;	// Return _everything_ that the file returns
			constexpr int noMessageHandler = 0;
			if (lua_pcall(o_luaState, argumentCount, returnValueCount, noMessageHandler) != LUA_OK)
			{
				Logging::OutputError(lua_tostring(o_luaState, -1));
				lua_pop(o_luaState, 1);
				result = Results::InvalidFile;
				return result;
			}

			// A well-behaved asset file will only return a single value
			const auto returnedValueCount = lua_gettop(o_luaState) - stackTopBeforeLoad;
			if (returnedValueCount != 1)
			{
				Logging::OutputError("Asset files must return a single table (instead of %d values)", returnedValueCount);
				lua_pop(o_luaState, returnedValueCount);
				result = Results::InvalidFile;
				return result;
			}

			// A correct asset file _must_ return a table
			if (!lua_istable(o_luaState, -1))
			{
				Logging::OutputError("Asset files must return a table (instead of a %s)", luaL_typename(o_luaState, -1));
				lua_pop(o_luaState, 1);
				result = Results::InvalidFile;
				return result;
			}
		}

		return Results::Success;
	}

	// Camera
	//-------

	cResult LoadCamera(lua_State& io_luaState, const char* i_filename, sSerializedCamera& o_serializedCamera)
	{
		// top: { camera = { ... }, positionStrategy = { ... }, ... }

		{
			constexpr const char* cameraKey = "camera";
			LUA_GET_FROM_DICT(io_luaState, cameraKey);
			// top: { fieldOfViewRadians = { ... }, zNearPlane = 0.0, ... }

			if (!lua_istable(&io_luaState, -1))
			{
				EAE6320_ASSERTF(false, "Expected dict from key \"%s\", received %s", cameraKey, lua_typename(&io_luaState, -1));
				return Results::InvalidFile;
			}

			if (!LoadCameraInfo(io_luaState, o_serializedCamera))
			{
				EAE6320_ASSERTF(false, "Failed to load camera info from key %s, from file %s", cameraKey, i_filename);
				return Results::InvalidFile;
			}
		}

		{
			constexpr const char* positionStrategyKey = "positionStrategy";
			LUA_GET_FROM_DICT(io_luaState, positionStrategyKey);
			// top: { type = "Follow", ... }

			if (!lua_istable(&io_luaState, -1))
			{
				EAE6320_ASSERTF(false, "Expected dict from key \"%s\", received %s", positionStrategyKey, lua_typename(&io_luaState, -1));
				return Results::InvalidFile;
			}

			if (!LoadSerializedPositionStrategy(io_luaState, o_serializedCamera))
			{
				EAE6320_ASSERTF(false, "Failed to load camera position strategy from key %s, from file %s", positionStrategyKey, i_filename);
				return Results::InvalidFile;
			}
		}

		{
			constexpr const char* orientationStrategyKey = "orientationStrategy";
			LUA_GET_FROM_DICT(io_luaState, orientationStrategyKey);
			// top: { type = "Follow", ... }

			if (!lua_istable(&io_luaState, -1))
			{
				EAE6320_ASSERTF(false, "Expected dict from key \"%s\", received %s", orientationStrategyKey, lua_typename(&io_luaState, -1));
				return Results::InvalidFile;
			}

			if (!LoadSerializedOrientationStrategy(io_luaState, o_serializedCamera))
			{
				EAE6320_ASSERTF(false, "Failed to load camera orientation strategy from key %s, from file %s", orientationStrategyKey, i_filename);
				return Results::InvalidFile;
			}
		}

		return Results::Success;
	}

	cResult LoadCameraInfo(lua_State& io_luaState, sSerializedCamera& o_serializedCamera)
	{
		// top: { fieldOfViewRadians = { ... }, zNearPlane = 0.0, ... }

		Camera::cTrackingCamera camera;

		Math::sVector2D fieldOfViewRadians;
		if (LoadVector2D(io_luaState, "fieldOfViewRadians", fieldOfViewRadians))
		{
			camera.SetFieldOfViewRadians(fieldOfViewRadians);
		}

		float zNearPlane = 0.f;
		if (LoadFloat(io_luaState, "zNearPlane", zNearPlane))
		{
			camera.SetZNearPlane(zNearPlane);
		}

		float zFarPlane = 0.f;
		if (LoadFloat(io_luaState, "zFarPlane", zFarPlane))
		{
			camera.SetZFarPlane(zFarPlane);
		}

		o_serializedCamera.serializedCameraSize = Serialization::GetSerializedSize<Camera::cTrackingCamera>();
		o_serializedCamera.serializedCamera.reserve(o_serializedCamera.serializedCameraSize);
		Serialization::Serialize(camera, o_serializedCamera.serializedCamera.data());

		//

		return Results::Success;
	}

	// Position strategies
	//--------------------

	cResult LoadSerializedPositionStrategy(lua_State& io_luaState, sSerializedCamera& o_serializedCamera)
	{
		const char* strategy = nullptr;
		LoadStr(io_luaState, "type", strategy);

		int16_t& o_classId = o_serializedCamera.positionStrategyClassId;
		std::string& o_serializedData = o_serializedCamera.serializedPositionStrategy;
		size_t& o_serializedSize = o_serializedCamera.serializedPositionStrategySize;

		if (strcmp(strategy, "Composer") == 0)
		{
			if (!LoadAndSerializePositionStrategy<Camera::cPositionComposer>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load position composer");
				return Results::InvalidFile;
			}
		}
		else if (strcmp(strategy, "Follow") == 0)
		{
			if (!LoadAndSerializePositionStrategy<Camera::cPositionFollow>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load position follow");
				return Results::InvalidFile;
			}
		}
		else if (strcmp(strategy, "LockToTarget") == 0)
		{
			if (!LoadAndSerializePositionStrategy<Camera::cPositionLockToTarget>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load position lock to target");
				return Results::InvalidFile;
			}
		}
		else if (strcmp(strategy, "Unmodified") == 0)
		{
			if (!LoadAndSerializePositionStrategy<Camera::cPositionUnmodified>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load position unmodified");
				return Results::InvalidFile;
			}
		}
		else
		{
			EAE6320_ASSERTF(false, "Position strategy %s does not match any existing strategies", strategy);
			return Results::InvalidFile;
		}

		return Results::Success;
	}

	// Orientation strategies
	//-----------------------

	cResult LoadSerializedOrientationStrategy(lua_State& io_luaState, sSerializedCamera& o_serializedCamera)
	{
		const char* strategy = nullptr;
		LoadStr(io_luaState, "type", strategy);

		int16_t& o_classId = o_serializedCamera.orientationStrategyClassId;
		std::string& o_serializedData = o_serializedCamera.serializedOrientationStrategy;
		size_t& o_serializedSize = o_serializedCamera.serializedOrientationStrategySize;

		if (strcmp(strategy, "Composer") == 0)
		{
			if (!LoadAndSerializeOrientationStrategy<Camera::cOrientationComposer>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load orientation composer");
				return Results::InvalidFile;
			}
		}
		else if (strcmp(strategy, "HardLookAt") == 0)
		{
			if (!LoadAndSerializeOrientationStrategy<Camera::cOrientationHardLookAt>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load orientation hard look at");
				return Results::InvalidFile;
			}
		}
		else if (strcmp(strategy, "LockToTarget") == 0)
		{
			if (!LoadAndSerializeOrientationStrategy<Camera::cOrientationLockToTarget>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load orientation lock to target");
				return Results::InvalidFile;
			}
		}
		else if (strcmp(strategy, "Unmodified") == 0)
		{
			if (!LoadAndSerializeOrientationStrategy<Camera::cOrientationUnmodified>(io_luaState, o_classId, o_serializedSize, o_serializedData))
			{
				EAE6320_ASSERTF(false, "Failed to load orientation unmodified");
				return Results::InvalidFile;
			}
		}
		else
		{
			EAE6320_ASSERTF(false, "Orientation strategy %s does not match any existing strategies", strategy);
			return Results::InvalidFile;
		}

		return Results::Success;
	}
}