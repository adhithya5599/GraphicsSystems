#pragma once

/*
	This class represents a camera positioning strategy,
	that will move the camera's position behind the target,
	where "behind" is based on the target's rotation.
	This can be useful for third-person follow cameras.
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
	class cPositionFollow : public iPositionStrategy
	{
	public:
		cPositionFollow() = default;

		// See https://docs.unity3d.com/Packages/com.unity.cinemachine@3.0/manual/CinemachineThirdPersonFollow.html
		// for more details, as this strategy is based on Cinemachine's Third Person Follow.

		// The direction that is (typically) upwards for the target.
		const Math::sVector& GetTargetUpDirection() const;
		void SetTargetUpDirection(const Math::sVector& i_targetUpDirection);

		// Damping z-value damps relative to the target's current forward vector.
		// Damping y-value damps relative to the target's current up vector (NOT m_targetUpDirection).
		// Damping x-value damps relative to the target's current left/right vector.
		const Math::sVector& GetDamping() const;
		void SetDamping(const Math::sVector& i_damping);

		// Position (local to the target) of the target's shoulder.
		// The shoulder rotates with the target about m_targetUpDirection
		// (e.g. if m_targetUpDirection is the up vector, then the shoulder rotates about the y-axis).
		const Math::sVector& GetShoulderOffset() const;
		void SetShoulderOffset(const Math::sVector& i_shoulderOffset);

		// The arm starts from the target's shoulder. At the end of the arm is the hand.
		// The target's current up vector (NOT m_targetUpDirection) is the direction of the arm.
		float GetArmLength() const;
		void SetArmLength(float i_armLength);

		// The camera's desired position is relative the hand,
		// specifically (camera distance) * (the target's current forward vector) behind the hand.
		float GetCameraDistance() const;
		void SetCameraDistance(float i_cameraDistance);

		Math::sVector CalculatePosition(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) override;

	private:
		Math::sVector m_targetUpDirection = Math::sVector(0.f, 1.f, 0.f);
		Math::sVector m_damping = Math::sVector();
		Math::sVector m_shoulderOffset = Math::sVector();
		float m_armLength = 0.f;
		float m_cameraDistance = 0.f;
	};
}

namespace eae6320::Serialization
{
	struct sSerializedPositionFollow
	{
		Math::sVector targetUpDirection;
		Math::sVector damping;
		Math::sVector shoulderOffset;
		float armLength;
		float cameraDistance;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cPositionFollow>()
	{
		return sizeof(sSerializedPositionFollow);
	}

	template <>
	void Serialize<Camera::cPositionFollow>(const Camera::cPositionFollow& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cPositionFollow>(const char* i_buffer, Camera::cPositionFollow& io_toDeserialize);
}