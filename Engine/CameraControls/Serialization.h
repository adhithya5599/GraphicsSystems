#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <Engine/Results/Results.h>
#include "Position/iPositionStrategy.h"
#include "Orientation/iOrientationStrategy.h"

namespace eae6320::Camera
{
	class cPositionUnmodified;
	class cPositionLockToTarget;
	class cPositionFollow;
	class cPositionComposer;

	class cOrientationUnmodified;
	class cOrientationLockToTarget;
	class cOrientationHardLookAt;
	class cOrientationComposer;
}

namespace eae6320::Serialization
{
	struct sCameraFileMetadata
	{
		static constexpr uint32_t DefaultMagicNumber = 0xdeadbeef;

		uint32_t magicNumber = DefaultMagicNumber;
		uint32_t checksum = 0;
		int16_t positionStrategyClassId = 0;
		int16_t orientationStrategyClassId = 0;
		uint32_t cameraDataSize = 0;
		uint32_t positionStrategySize = 0;
		uint32_t orientationStrategySize = 0;
	};

	// These functions allow serialization of an existing iPositionStrategy/iOrientationStrategy
	// and deserialization of data into an existing (presumably blank) iPositionStrategy/iOrientationStrategy.
	// If you make your own iPositionStrategy or iOrientationStrategy implementation,
	// you'll have to implement serialization/deserialization, if you want it to work with the asset build system.
	
	// To do this:
	// 1. Declare template overloads for the following functions and implement them.

	template <typename T>
	constexpr size_t GetSerializedSize()
	{
		return sizeof(T);
	}

	template <typename T>
	void Serialize(const T& i_toSerialize, char* io_buffer)
	{
		memcpy(io_buffer, &i_toSerialize, GetSerializedSize<T>());
	}

	template <typename T>
	void Deserialize(const char* i_buffer, T& io_toDeserialize)
	{
		memcpy(&io_toDeserialize, i_buffer, GetSerializedSize<T>());
	}

	// 2. Declare a template overload of GetClassID. Make sure its return value is unique from the others.

	template <typename T> constexpr int16_t GetClassID() { return -1; }

	template <> constexpr int16_t GetClassID<Camera::cPositionUnmodified>() { return 1; }
	template <> constexpr int16_t GetClassID<Camera::cPositionLockToTarget>() { return 2; }
	template <> constexpr int16_t GetClassID<Camera::cPositionFollow>() { return 3; }
	template <> constexpr int16_t GetClassID<Camera::cPositionComposer>() { return 4; }

	template <> constexpr int16_t GetClassID<Camera::cOrientationUnmodified>() { return 5; }
	template <> constexpr int16_t GetClassID<Camera::cOrientationLockToTarget>() { return 6; }
	template <> constexpr int16_t GetClassID<Camera::cOrientationHardLookAt>() { return 7; }
	template <> constexpr int16_t GetClassID<Camera::cOrientationComposer>() { return 8; }

	cResult LoadCameraFromFile(const char* i_infile, Camera::cTrackingCamera& o_trackingCamera);

	uint32_t ComputeChecksum(const sCameraFileMetadata& i_metadata,
		const void* i_cameraData, const void* i_positionStrategyData, const void* i_orientationStrategyData);

	template <typename T>
	std::unique_ptr<T> ConstructAndDeserialize(const char* i_buffer)
	{
		std::unique_ptr<T> objectPtr = std::make_unique<T>();

		Deserialize(i_buffer, *objectPtr);

		return objectPtr;
	}

	// 3. In the CameraBuilder project (cCameraBuilder.cpp),
	// make a function that converts Lua data into your position/orientations strategy object.
}