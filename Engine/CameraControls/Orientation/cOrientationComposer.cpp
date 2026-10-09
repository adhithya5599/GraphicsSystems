#include "cOrientationComposer.h"
#include <cmath>
#include <limits>
#include <Engine/Math/sVector.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sRectangle.h>
#include "../cTrackingCamera.h"

namespace eae6320::Camera
{
	const Math::sRectangle& cOrientationComposer::GetDeadZone() const
	{
		return m_deadZone;
	}

	void cOrientationComposer::SetDeadZone(const Math::sRectangle& i_deadZone)
	{
		m_deadZone = i_deadZone;
	}

	void cOrientationComposer::SetDeadZone(const Math::sVector2D& i_deadZone)
	{
		m_deadZone = Math::sRectangle(-i_deadZone.x, i_deadZone.x, -i_deadZone.y, i_deadZone.y);
	}

	const Math::sRectangle& cOrientationComposer::GetSoftZone() const
	{
		return m_softZone;
	}

	void cOrientationComposer::SetSoftZone(const Math::sRectangle& i_softZone)
	{
		m_softZone = i_softZone;
	}

	void cOrientationComposer::SetSoftZone(const Math::sVector2D& i_softZone)
	{
		m_softZone = Math::sRectangle(-i_softZone.x, i_softZone.x, -i_softZone.y, i_softZone.y);
	}

	const Math::sVector2D& cOrientationComposer::GetTargetOffset() const
	{
		return m_targetOffset;
	}

	void cOrientationComposer::SetTargetOffset(const Math::sVector2D& i_targetOffset)
	{
		m_targetOffset = i_targetOffset;
	}

	const Math::sVector& cOrientationComposer::GetCameraUpDirectionOverride() const
	{
		return m_cameraUpDirectionOverride;
	}

	void cOrientationComposer::SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride)
	{
		m_cameraUpDirectionOverride = i_cameraUpDirectionOverride;
	}

	bool cOrientationComposer::IsCameraUpDirectionOverridden() const
	{
		return m_cameraUpDirectionOverride != Math::sVector();
	}

	void cOrientationComposer::ResetCameraUpDirectionOverride()
	{
		SetCameraUpDirectionOverride(Math::sVector());
	}

	float cOrientationComposer::GetDamping() const
	{
		return m_damping;
	}

	void cOrientationComposer::SetDamping(float i_damping)
	{
		m_damping = i_damping;
	}

	Math::cQuaternion cOrientationComposer::CalculateOrientation(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		const Math::sVector2D& fieldOfViewRadians = i_trackingCamera.GetFieldOfViewRadians();

		// All screen space positions are relative to the target offset
		Math::sRectangle deadZone = m_deadZone - GetTargetOffset();
		Math::sRectangle softZone = m_softZone - GetTargetOffset();

		// Where the target would be in screen space, then those positions after soft/dead zones are applied
		Math::sVector2D targetScreenPos = Math::WorldSpaceToScreenSpace(
			i_targetPosition, i_cameraPosition, i_cameraOrientation, fieldOfViewRadians) - GetTargetOffset();
		Math::sVector2D screenPosDeadZone(
			Math::Clamp(targetScreenPos.x, deadZone.left, deadZone.right),
			Math::Clamp(targetScreenPos.y, deadZone.bottom, deadZone.top));
		Math::sVector2D screenPosSoftZone(
			Math::Clamp(targetScreenPos.x, softZone.left, softZone.right),
			Math::Clamp(targetScreenPos.y, softZone.bottom, softZone.top));

		if (Math::sVector2D::Approximately(screenPosDeadZone, targetScreenPos))
		{
			// Target already in dead zone, no need to move camera
			return i_cameraOrientation;
		}

		Math::sVector2D screenDeltaDeadZone = targetScreenPos - screenPosDeadZone;
		Math::sVector2D screenDeltaSoftZone = targetScreenPos - screenPosSoftZone;

		// Convert back to world space
		Math::sVector directionForDeadZone = Math::ScreenSpaceToWorldSpace(
			screenDeltaDeadZone, i_cameraPosition, i_cameraOrientation, fieldOfViewRadians) - i_cameraPosition;
		Math::sVector directionForSoftZone = Math::ScreenSpaceToWorldSpace(
			screenDeltaSoftZone, i_cameraPosition, i_cameraOrientation, fieldOfViewRadians) - i_cameraPosition;

		Math::cMatrix_transformation camOrientationTransformation(i_cameraOrientation, Math::sVector());
		Math::sVector cameraUp = IsCameraUpDirectionOverridden() ? m_cameraUpDirectionOverride : camOrientationTransformation.GetUpDirection();

		// Look at where the object would be in the soft zone (since we must be in the soft zone),
		// then move rotation towards looking at where it'd be in the dead zone
		Math::cQuaternion orientationForDeadZone = Math::cQuaternion::LookRotation(directionForDeadZone, cameraUp);
		Math::cQuaternion orientationForSoftZone = Math::cQuaternion::LookRotation(directionForSoftZone, cameraUp);

		float time = Math::Damp(0, 1, m_damping, i_deltaSeconds);
		Math::cQuaternion result = Math::cQuaternion::Slerp(orientationForSoftZone, orientationForDeadZone, time);

		return result;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cOrientationComposer>(const Camera::cOrientationComposer& i_toSerialize, char* io_buffer)
	{
		sSerializedOrientationComposer* serialized = reinterpret_cast<sSerializedOrientationComposer*>(io_buffer);

		serialized->deadZone = i_toSerialize.GetDeadZone();
		serialized->softZone = i_toSerialize.GetSoftZone();
		serialized->targetOffset = i_toSerialize.GetTargetOffset();
		serialized->cameraUpDirectionOverride = i_toSerialize.GetCameraUpDirectionOverride();
		serialized->damping = i_toSerialize.GetDamping();
	}

	template <>
	void Deserialize<Camera::cOrientationComposer>(const char* i_buffer, Camera::cOrientationComposer& io_toDeserialize)
	{
		const sSerializedOrientationComposer* serialized = reinterpret_cast<const sSerializedOrientationComposer*>(i_buffer);

		io_toDeserialize.SetDeadZone(serialized->deadZone);
		io_toDeserialize.SetSoftZone(serialized->softZone);
		io_toDeserialize.SetTargetOffset(serialized->targetOffset);
		io_toDeserialize.SetCameraUpDirectionOverride(serialized->cameraUpDirectionOverride);
		io_toDeserialize.SetDamping(serialized->damping);
	}
}