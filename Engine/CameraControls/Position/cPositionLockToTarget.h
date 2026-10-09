#pragma once

/*
	This class represents a camera positioning strategy,
	that will move the camera's position to the target's position.
	This strategy could be seen as a simpler version of cPositionFollow.
	This can be useful for a first-person camera.
*/

#include <Engine/Math/sVector.h>
#include "iPositionStrategy.h"
#include "../Serialization.h"

namespace eae6320::Math
{
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cPositionLockToTarget : public iPositionStrategy
	{
	public:
		cPositionLockToTarget() = default;

		// If damping is nonzero, then the camera won't immediately snap its position to the target.
		// Instead, it will smoothly move towards the target from its current position.
		// Higher values of damping will make the camera move slower towards the target.
		const Math::sVector& GetDamping() const;
		void SetDamping(const Math::sVector& i_damping);

		// By default, damping directions are the world xyz-axes.
		// Setting this to true will instead make the damping directions
		// the target's right (x), up (y), forward (z) vectors (based on the target's current rotation).
		bool IsDampingDirectionsRelativeToTarget() const;
		void SetDampingDirectionsRelativeToTarget(bool i_dampingDirectionsRelativeToTarget);

		Math::sVector CalculatePosition(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) override;

	private:
		Math::sVector m_damping = Math::sVector();
		bool m_dampingDirectionsRelativeToTarget = false;
	};
}

namespace eae6320::Serialization
{
	struct sSerializedPositionLockToTarget
	{
		Math::sVector damping = Math::sVector();
		uint32_t dampingDirectionsRelativeToTarget;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cPositionLockToTarget>()
	{
		return sizeof(sSerializedPositionLockToTarget);
	}

	template <>
	void Serialize<Camera::cPositionLockToTarget>(const Camera::cPositionLockToTarget& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cPositionLockToTarget>(const char* i_buffer, Camera::cPositionLockToTarget& io_toDeserialize);
}