#include "cOrientationUnmodified.h"
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>


namespace eae6320::Camera
{
	Math::cQuaternion cOrientationUnmodified::CalculateOrientation(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		return i_cameraOrientation;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cOrientationUnmodified>(const Camera::cOrientationUnmodified& i_toSerialize, char* io_buffer)
	{
		// No need to serialize anything
	}

	template <>
	void Deserialize<Camera::cOrientationUnmodified>(const char* i_buffer, Camera::cOrientationUnmodified& io_toDeserialize)
	{
		// No need to deserialize anything
	}
}