#include "cOrientationLockToTarget.h"
#include <cmath>
#include <limits>
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/cQuaternion.h>

namespace eae6320::Camera
{

	const Math::sVector& cOrientationLockToTarget::GetCameraUpDirectionOverride() const
	{
		return m_cameraUpDirectionOverride;
	}

	void cOrientationLockToTarget::SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride)
	{
		m_cameraUpDirectionOverride = i_cameraUpDirectionOverride;
	}

	bool cOrientationLockToTarget::IsCameraUpDirectionOverridden() const
	{
		return GetCameraUpDirectionOverride() != Math::sVector();
	}

	void cOrientationLockToTarget::ResetCameraUpDirectionOverride()
	{
		SetCameraUpDirectionOverride(Math::sVector());
	}

	float cOrientationLockToTarget::GetDamping() const
	{
		return m_damping;
	}

	void cOrientationLockToTarget::SetDamping(float i_damping)
	{
		m_damping = i_damping;
	}

	Math::cQuaternion cOrientationLockToTarget::CalculateOrientation(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		Math::cQuaternion targetOrientation;
		if (IsCameraUpDirectionOverridden())
		{
			Math::sVector targetForward = i_targetOrientation.CalculateForwardDirection();
			targetOrientation = Math::cQuaternion::LookRotation(targetForward, GetCameraUpDirectionOverride());
		}
		else
		{
			targetOrientation = i_targetOrientation;
		}

		if (Math::Approximately(m_damping, 0.f))
		{
			// If damping is zero, we can just snap to the target
			return targetOrientation;
		}

		Math::cQuaternion camOrientation = Math::cQuaternion(i_cameraOrientation);

		float time = Math::Damp(0, 1, m_damping, i_deltaSeconds);
		Math::cQuaternion result = Math::cQuaternion::Slerp(camOrientation, targetOrientation, time);

		return result;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cOrientationLockToTarget>(const Camera::cOrientationLockToTarget& i_toSerialize, char* io_buffer)
	{
		sSerializedOrientationLockToTarget* serialized = reinterpret_cast<sSerializedOrientationLockToTarget*>(io_buffer);

		serialized->cameraUpDirectionOverride = i_toSerialize.GetCameraUpDirectionOverride();
		serialized->damping = i_toSerialize.GetDamping();
	}

	template <>
	void Deserialize<Camera::cOrientationLockToTarget>(const char* i_buffer, Camera::cOrientationLockToTarget& io_toDeserialize)
	{
		const sSerializedOrientationLockToTarget* serialized = reinterpret_cast<const sSerializedOrientationLockToTarget*>(i_buffer);

		io_toDeserialize.SetCameraUpDirectionOverride(serialized->cameraUpDirectionOverride);
		io_toDeserialize.SetDamping(serialized->damping);
	}
}