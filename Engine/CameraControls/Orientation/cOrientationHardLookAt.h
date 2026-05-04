#pragma once

/*
	This class represents a camera orientation strategy,
	that will move the camera's rotation to keep the target at the center of the frame.
	This strategy could be seen as a simpler version of cOrientationComposer.
*/

#include <Engine/Math/sVector.h>
#include "iOrientationStrategy.h"
#include "../Serialization.h"

namespace eae6320::Math
{
	struct sVector;
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cOrientationHardLookAt : public iOrientationStrategy
	{
	public:
		cOrientationHardLookAt() = default;

		// See cOrientationComposer.h for details about camera up direction override

		const Math::sVector& GetCameraUpDirectionOverride() const;
		void SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride);

		bool IsCameraUpDirectionOverridden() const;
		void ResetCameraUpDirectionOverride();

		float GetDamping() const;
		void SetDamping(float i_damping);

		Math::cQuaternion CalculateOrientation(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) override;

	private:
		Math::sVector m_cameraUpDirectionOverride = Math::sVector();
		float m_damping = 0.f;
	};
}

namespace eae6320::Serialization
{
	struct sSerializedOrientationHardLookAt
	{
		Math::sVector cameraUpDirectionOverride;
		float damping;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cOrientationHardLookAt>()
	{
		return sizeof(sSerializedOrientationHardLookAt);
	}

	template <>
	void Serialize<Camera::cOrientationHardLookAt>(const Camera::cOrientationHardLookAt& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cOrientationHardLookAt>(const char* i_buffer, Camera::cOrientationHardLookAt& io_toDeserialize);
}