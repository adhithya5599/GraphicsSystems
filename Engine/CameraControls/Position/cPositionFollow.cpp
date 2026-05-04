#include "cPositionFollow.h"
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/cQuaternion.h>

namespace eae6320::Camera
{
	const Math::sVector& cPositionFollow::GetTargetUpDirection() const
	{
		return m_targetUpDirection;
	}

	void cPositionFollow::SetTargetUpDirection(const Math::sVector& i_targetUpDirection)
	{
		m_targetUpDirection = i_targetUpDirection;
	}

	const Math::sVector& cPositionFollow::GetDamping() const
	{
		return m_damping;
	}

	void cPositionFollow::SetDamping(const Math::sVector& i_damping)
	{
		m_damping = i_damping;
	}

	const Math::sVector& cPositionFollow::GetShoulderOffset() const
	{
		return m_shoulderOffset;
	}

	void cPositionFollow::SetShoulderOffset(const Math::sVector& i_shoulderOffset)
	{
		m_shoulderOffset = i_shoulderOffset;
	}

	float cPositionFollow::GetArmLength() const
	{
		return m_armLength;
	}

	void cPositionFollow::SetArmLength(float i_armLength)
	{
		m_armLength = i_armLength;
	}

	float cPositionFollow::GetCameraDistance() const
	{
		return m_cameraDistance;
	}

	void cPositionFollow::SetCameraDistance(float i_cameraDistance)
	{
		m_cameraDistance = i_cameraDistance;
	}

	Math::sVector cPositionFollow::CalculatePosition(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		Math::cMatrix_transformation targetTransformation
			= Math::cMatrix_transformation(i_targetOrientation, Math::sVector());
		Math::sVector targetForward = -targetTransformation.GetBackDirection();
		Math::sVector targetForwardHorizontal = Math::sVector::ProjectOntoPlane(targetForward, GetTargetUpDirection());
		Math::sVector targetUp = targetTransformation.GetUpDirection();

		Math::cQuaternion horizontalRotation = Math::cQuaternion::LookRotation(targetForwardHorizontal, GetTargetUpDirection());

		Math::sVector shoulder = (horizontalRotation * GetShoulderOffset()) + i_targetPosition;
		Math::sVector hand = shoulder + targetUp * GetArmLength();
		Math::sVector desiredCamera = hand - targetForward * GetCameraDistance();

		Math::sVector cameraDelta = desiredCamera - i_cameraPosition;
		Math::sVector deltaZ = Math::sVector::Project(cameraDelta, targetForward);
		Math::sVector deltaY = Math::sVector::Project(cameraDelta, targetUp);
		Math::sVector deltaX = cameraDelta - deltaZ - deltaY;

		Math::sVector deltaZDamped = Math::sVector::Damp(Math::sVector(), deltaZ, GetDamping().z, i_deltaSeconds);
		Math::sVector deltaYDamped = Math::sVector::Damp(Math::sVector(), deltaY, GetDamping().y, i_deltaSeconds);
		Math::sVector deltaXDamped = Math::sVector::Damp(Math::sVector(), deltaX, GetDamping().x, i_deltaSeconds);

		return i_cameraPosition + deltaXDamped + deltaYDamped + deltaZDamped;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cPositionFollow>(const Camera::cPositionFollow& i_toSerialize, char* io_buffer)
	{
		sSerializedPositionFollow* serialized = reinterpret_cast<sSerializedPositionFollow*>(io_buffer);

		serialized->targetUpDirection = i_toSerialize.GetTargetUpDirection();
		serialized->damping = i_toSerialize.GetDamping();
		serialized->shoulderOffset = i_toSerialize.GetShoulderOffset();
		serialized->armLength = i_toSerialize.GetArmLength();
		serialized->cameraDistance = i_toSerialize.GetCameraDistance();
	}

	template <>
	void Deserialize<Camera::cPositionFollow>(const char* i_buffer, Camera::cPositionFollow& io_toDeserialize)
	{
		const sSerializedPositionFollow* serialized = reinterpret_cast<const sSerializedPositionFollow*>(i_buffer);

		io_toDeserialize.SetTargetUpDirection(serialized->targetUpDirection);
		io_toDeserialize.SetDamping(serialized->damping);
		io_toDeserialize.SetShoulderOffset(serialized->shoulderOffset);
		io_toDeserialize.SetArmLength(serialized->armLength);
		io_toDeserialize.SetCameraDistance(serialized->cameraDistance);
	}
}