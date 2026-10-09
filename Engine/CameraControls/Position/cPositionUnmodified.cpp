#include "cPositionUnmodified.h"
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>

namespace eae6320::Camera
{
	Math::sVector cPositionUnmodified::CalculatePosition(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		return i_cameraPosition;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cPositionUnmodified>(const Camera::cPositionUnmodified& i_toSerialize, char* io_buffer)
	{
		// No need to serialize anything
	}

	template <>
	void Deserialize<Camera::cPositionUnmodified>(const char* i_buffer, Camera::cPositionUnmodified& io_toDeserialize)
	{
		// No need to deserialize anything
	}
}