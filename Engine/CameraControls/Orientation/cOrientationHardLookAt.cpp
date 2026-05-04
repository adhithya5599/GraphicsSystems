#include "cOrientationHardLookAt.h"
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/cQuaternion.h>

namespace eae6320::Camera
{
	const Math::sVector& cOrientationHardLookAt::GetCameraUpDirectionOverride() const
	{
		return m_cameraUpDirectionOverride;
	}

	void cOrientationHardLookAt::SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride)
	{
		m_cameraUpDirectionOverride = i_cameraUpDirectionOverride;
	}

	bool cOrientationHardLookAt::IsCameraUpDirectionOverridden() const
	{
		return GetCameraUpDirectionOverride() != Math::sVector();
	}

	void cOrientationHardLookAt::ResetCameraUpDirectionOverride()
	{
		SetCameraUpDirectionOverride(Math::sVector());
	}

	float cOrientationHardLookAt::GetDamping() const
	{
		return m_damping;
	}

	void cOrientationHardLookAt::SetDamping(float i_damping)
	{
		m_damping = i_damping;
	}

	Math::cQuaternion cOrientationHardLookAt::CalculateOrientation(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		Math::sVector lookDirection = i_targetPosition - i_cameraPosition;

		Math::cMatrix_transformation camOrientationTransformation(i_cameraOrientation, Math::sVector());
		Math::sVector cameraUp = IsCameraUpDirectionOverridden()
			? GetCameraUpDirectionOverride()
			: camOrientationTransformation.GetUpDirection();

		Math::cQuaternion desiredOrientation = Math::cQuaternion::LookRotation(lookDirection, cameraUp);

		float time = Math::Damp(0, 1, m_damping, i_deltaSeconds);
		Math::cQuaternion result = Math::cQuaternion::Slerp(i_cameraOrientation, desiredOrientation, time);

		return result;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cOrientationHardLookAt>(const Camera::cOrientationHardLookAt& i_toSerialize, char* io_buffer)
	{
		sSerializedOrientationHardLookAt* serialized = reinterpret_cast<sSerializedOrientationHardLookAt*>(io_buffer);

		serialized->cameraUpDirectionOverride = i_toSerialize.GetCameraUpDirectionOverride();
		serialized->damping = i_toSerialize.GetDamping();
	}

	template <>
	void Deserialize<Camera::cOrientationHardLookAt>(const char* i_buffer, Camera::cOrientationHardLookAt& io_toDeserialize)
	{
		const sSerializedOrientationHardLookAt* serialized = reinterpret_cast<const sSerializedOrientationHardLookAt*>(i_buffer);

		io_toDeserialize.SetCameraUpDirectionOverride(serialized->cameraUpDirectionOverride);
		io_toDeserialize.SetDamping(serialized->damping);
	}
}