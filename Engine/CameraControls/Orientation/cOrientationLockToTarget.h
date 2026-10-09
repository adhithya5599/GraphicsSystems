#pragma once

/*
	This class represents a camera orientation strategy,
	that will move the camera's rotation to the target's rotation.
	This can be useful for a first-person camera.
*/

#include <Engine/Math/sVector.h>
#include "iOrientationStrategy.h"
#include "../Serialization.h"

namespace eae6320::Math
{
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cOrientationLockToTarget : public iOrientationStrategy
	{
	public:
		cOrientationLockToTarget() = default;

		// See cOrientationComposer.h for details about camera up direction override

		const Math::sVector& GetCameraUpDirectionOverride() const;
		void SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride);

		bool IsCameraUpDirectionOverridden() const;
		void ResetCameraUpDirectionOverride();

		// If damping is nonzero, then the camera won't immediately snap its rotation to the target.
		// Instead, it will smoothly move its rotation towards the target from its current rotation.
		// Higher values of damping will make the camera rotate slower towards the target.
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
	struct sSerializedOrientationLockToTarget
	{
		Math::sVector cameraUpDirectionOverride;
		float damping;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cOrientationLockToTarget>()
	{
		return sizeof(sSerializedOrientationLockToTarget);
	}

	template <>
	void Serialize<Camera::cOrientationLockToTarget>(const Camera::cOrientationLockToTarget& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cOrientationLockToTarget>(const char* i_buffer, Camera::cOrientationLockToTarget& io_toDeserialize);
}