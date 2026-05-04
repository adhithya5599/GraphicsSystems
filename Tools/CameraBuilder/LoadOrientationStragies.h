#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <Engine/CameraControls/Orientation/cOrientationComposer.h>
#include <Engine/CameraControls/Orientation/cOrientationHardLookAt.h>
#include <Engine/CameraControls/Orientation/cOrientationLockToTarget.h>
#include <Engine/CameraControls/Orientation/cOrientationUnmodified.h>
#include <Engine/CameraControls/Orientation/iOrientationStrategy.h>
#include <Engine/CameraControls/Serialization.h>
#include <Engine/Asserts/Asserts.h>
#include <Engine/Results/Results.h>
#include <External/Lua/Includes.h>
#include "LoadValues.h"

namespace eae6320::Assets
{
	template <typename T>
	cResult LoadOrientationStrategy(lua_State& io_luaState, T& o_orientationStrategy)
	{
		return Results::Failure;
	}

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationComposer>(lua_State& io_luaState, Camera::cOrientationComposer& o_orientationComposer);

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationHardLookAt>(lua_State& io_luaState, Camera::cOrientationHardLookAt& o_orientationHardLookAt);

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationLockToTarget>(lua_State& io_luaState, Camera::cOrientationLockToTarget& o_orientationLockToTarget);

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationUnmodified>(lua_State& io_luaState, Camera::cOrientationUnmodified& o_orientationUnmodified);

	template <typename T>
	cResult LoadAndSerializeOrientationStrategy(lua_State& io_luaState, int16_t& o_classId, size_t& o_serializedSize, std::string& o_serializedData)
	{
		std::unique_ptr<T> orientationStrategy = std::make_unique<T>();

		if (!LoadOrientationStrategy<T>(io_luaState, *orientationStrategy))
		{
			EAE6320_ASSERTF(false, "Failed to load orientation strategy");
			return Results::InvalidFile;
		}

		o_classId = Serialization::GetClassID<T>();
		o_serializedSize = Serialization::GetSerializedSize<T>();
		o_serializedData.reserve(o_serializedSize);
		Serialization::Serialize(*orientationStrategy, o_serializedData.data());

		return Results::Success;
	}
}