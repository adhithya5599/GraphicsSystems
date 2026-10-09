#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <Engine/CameraControls/Position/cPositionComposer.h>
#include <Engine/CameraControls/Position/cPositionFollow.h>
#include <Engine/CameraControls/Position/cPositionLockToTarget.h>
#include <Engine/CameraControls/Position/cPositionUnmodified.h>
#include <Engine/CameraControls/Position/iPositionStrategy.h>
#include <Engine/CameraControls/Serialization.h>
#include <Engine/Asserts/Asserts.h>
#include <Engine/Results/Results.h>
#include <External/Lua/Includes.h>
#include "LoadValues.h"

namespace eae6320::Camera
{
	class cPositionUnmodified;
	class cPositionLockToTarget;
	class cPositionFollow;
	class cPositionComposer;
}

namespace eae6320::Assets
{
	template <typename T>
	cResult LoadPositionStrategy(lua_State& io_luaState, T& o_positionStrategy)
	{
		return Results::Failure;
	}

	template <>
	cResult LoadPositionStrategy<Camera::cPositionComposer>(lua_State& io_luaState, Camera::cPositionComposer& o_positionComposer);

	template <>
	cResult LoadPositionStrategy<Camera::cPositionFollow>(lua_State& io_luaState, Camera::cPositionFollow& o_positionFollow);

	template <>
	cResult LoadPositionStrategy<Camera::cPositionLockToTarget>(lua_State& io_luaState, Camera::cPositionLockToTarget& o_positionLockToTarget);

	template <>
	cResult LoadPositionStrategy<Camera::cPositionUnmodified>(lua_State& io_luaState, Camera::cPositionUnmodified& o_positionUnmodified);

	template <typename T>
	cResult LoadAndSerializePositionStrategy(lua_State& io_luaState, int16_t& o_classId, size_t& o_serializedSize, std::string& o_serializedData)
	{
		std::unique_ptr<T> positionStrategy = std::make_unique<T>();

		if (!LoadPositionStrategy<T>(io_luaState, *positionStrategy))
		{
			EAE6320_ASSERTF(false, "Failed to load position strategy");
			return Results::InvalidFile;
		}

		o_classId = Serialization::GetClassID<T>();
		o_serializedSize = Serialization::GetSerializedSize<T>();
		o_serializedData.reserve(o_serializedSize);
		Serialization::Serialize(*positionStrategy, o_serializedData.data());

		return Results::Success;
	}
}