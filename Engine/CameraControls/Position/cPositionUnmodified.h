#pragma once

/*
	This class represents a camera positioning strategy,
	that will simply keep the camera's position unmodified.
	This can be useful if you want to keep the camera's position stationary (e.g. security camera).
*/

#include "iPositionStrategy.h"
#include "../Serialization.h"

namespace eae6320::Math
{
	struct sVector;
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cPositionUnmodified : public iPositionStrategy
	{
	public:
		Math::sVector CalculatePosition(
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
	constexpr size_t GetSerializedSize<Camera::cPositionUnmodified>()
	{
		return 0;
	}

	template <>
	void Serialize<Camera::cPositionUnmodified>(const Camera::cPositionUnmodified& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cPositionUnmodified>(const char* i_buffer, Camera::cPositionUnmodified& io_toDeserialize);
}