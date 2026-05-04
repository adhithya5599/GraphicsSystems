#pragma once

/*
	This class represents a camera orientation strategy,
	that will simply keep the camera's rotation unmodified.
	This can be useful if you want to keep the camera's rotation stationary (e.g. top-down camera).
*/

#include "iOrientationStrategy.h"
#include "../Serialization.h"

namespace eae6320::Math
{
	struct sVector;
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cOrientationUnmodified : public iOrientationStrategy
	{
	public:
		Math::cQuaternion CalculateOrientation(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) override;
	};
}

namespace eae6320::Serialization
{
	template <>
	constexpr size_t GetSerializedSize<Camera::cOrientationUnmodified>()
	{
		return 0;
	}

	template <>
	void Serialize<Camera::cOrientationUnmodified>(const Camera::cOrientationUnmodified& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cOrientationUnmodified>(const char* i_buffer, Camera::cOrientationUnmodified& io_toDeserialize);
}