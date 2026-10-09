#include "cPositionComposer.h"
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/Functions.h>
#include "../cTrackingCamera.h"

namespace eae6320::Camera
{
	const Math::sRectangle& cPositionComposer::GetDeadZone() const
	{
		return m_deadZone;
	}

	void cPositionComposer::SetDeadZone(const Math::sRectangle& i_deadZone)
	{
		m_deadZone = i_deadZone;
	}

	void cPositionComposer::SetDeadZone(const Math::sVector2D& i_deadZone)
	{
		m_deadZone = Math::sRectangle(-i_deadZone.x, i_deadZone.x, -i_deadZone.y, i_deadZone.y);
	}

	const Math::sRectangle& cPositionComposer::GetSoftZone() const
	{
		return m_softZone;
	}

	void cPositionComposer::SetSoftZone(const Math::sRectangle& i_softZone)
	{
		m_softZone = i_softZone;
	}

	void cPositionComposer::SetSoftZone(const Math::sVector2D& i_softZone)
	{
		m_softZone = Math::sRectangle(-i_softZone.x, i_softZone.x, -i_softZone.y, i_softZone.y);
	}

	const Math::sVector2D& cPositionComposer::GetTargetOffset() const
	{
		return m_targetOffset;
	}

	void cPositionComposer::SetTargetOffset(const Math::sVector2D& i_targetOffset)
	{
		m_targetOffset = i_targetOffset;
	}

	const Math::sVector2D& cPositionComposer::GetDamping() const
	{
		return m_damping;
	}

	void cPositionComposer::SetDamping(const Math::sVector2D& i_damping)
	{
		m_damping = i_damping;
	}

	const Math::sVector& cPositionComposer::GetScreenPlaneNormalOverride() const
	{
		return m_screenPlaneNormalOverride;
	}

	void cPositionComposer::SetScreenPlaneNormalOverride(const Math::sVector& i_screenPlaneNormalOverride)
	{
		m_screenPlaneNormalOverride = i_screenPlaneNormalOverride;
	}

	bool cPositionComposer::IsScreenPlaneNormalOverridden() const
	{
		return GetScreenPlaneNormalOverride() != Math::sVector();
	}

	void cPositionComposer::ResetScreenPlaneNormalOverride()
	{
		SetScreenPlaneNormalOverride(Math::sVector());
	}

	const Math::sVector& cPositionComposer::GetCameraUpDirectionOverride() const
	{
		return m_cameraUpDirectionOverride;
	}

	void cPositionComposer::SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride)
	{
		m_cameraUpDirectionOverride = i_cameraUpDirectionOverride;
	}

	bool cPositionComposer::IsCameraUpDirectionOverridden() const
	{
		return GetCameraUpDirectionOverride() != Math::sVector();
	}

	void cPositionComposer::ResetCameraUpDirectionOverride()
	{
		SetCameraUpDirectionOverride(Math::sVector());
	}

	Math::sVector cPositionComposer::CalculatePosition(
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
			return i_cameraPosition;
		}

		// Convert back to world space: values are given in directions
		// (from the camera's position) where the positions could be in world space
		Math::sVector directionRaw = Math::ScreenSpaceToWorldSpace(
			targetScreenPos, i_cameraPosition, i_cameraOrientation, fieldOfViewRadians) - i_cameraPosition;
		Math::sVector directionDeadZone = Math::ScreenSpaceToWorldSpace(
			screenPosDeadZone, i_cameraPosition, i_cameraOrientation, fieldOfViewRadians) - i_cameraPosition;
		Math::sVector directionSoftZone = Math::ScreenSpaceToWorldSpace(
			screenPosSoftZone, i_cameraPosition, i_cameraOrientation, fieldOfViewRadians) - i_cameraPosition;

		// Get the normal of the plane P that the camera moves along
		Math::cMatrix_transformation camOrientationTransformation(i_cameraOrientation, Math::sVector());
		Math::sVector screenPlaneNormal = IsScreenPlaneNormalOverridden()
			? GetScreenPlaneNormalOverride()
			: - camOrientationTransformation.GetBackDirection();
		Math::sVector screenPlaneUp = IsCameraUpDirectionOverridden()
			? GetCameraUpDirectionOverride()
			: camOrientationTransformation.GetUpDirection();

		// Find the intersection between plane P' (same normal, but contains the target's position
		// instead of the camera's position) and the direction vectors
		float deltaDot = Math::Dot(i_targetPosition - i_cameraPosition, screenPlaneNormal);
		float rawMultiplier = deltaDot / Math::Dot(directionRaw, screenPlaneNormal);
		float deadZoneMultiplier = deltaDot / Math::Dot(directionDeadZone, screenPlaneNormal);
		float softZoneMultiplier = deltaDot / Math::Dot(directionSoftZone, screenPlaneNormal);
		Math::sVector targetSimulatedPosRaw = i_cameraPosition + directionRaw * rawMultiplier;
		Math::sVector targetSimulatedPosDeadZone = i_cameraPosition + directionDeadZone * deadZoneMultiplier;
		Math::sVector targetSimulatedPosSoftZone = i_cameraPosition + directionSoftZone * softZoneMultiplier;

		// Project those intersection positions back to plane P
		Math::sVector camPosDeadZone = i_cameraPosition - (targetSimulatedPosDeadZone - targetSimulatedPosRaw);
		Math::sVector camPosSoftZone = i_cameraPosition - (targetSimulatedPosSoftZone - targetSimulatedPosRaw);

		// Start our position at the soft zone position (since we must be in the soft zone),
		// then move toward the dead zone position
		Math::sVector delta = camPosDeadZone - camPosSoftZone;
		Math::sVector deltaVertical = Math::sVector::Project(delta, screenPlaneUp);
		Math::sVector deltaHorizontal = delta - deltaVertical;

		Math::sVector deltaVerticalDamped = Math::sVector::Damp(Math::sVector(), deltaVertical, m_damping.y, i_deltaSeconds);
		Math::sVector deltaHorizontalDamped = Math::sVector::Damp(Math::sVector(), deltaHorizontal, m_damping.x, i_deltaSeconds);

		return camPosSoftZone + deltaVerticalDamped + deltaHorizontalDamped;
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cPositionComposer>(const Camera::cPositionComposer& i_toSerialize, char* io_buffer)
	{
		sSerializedPositionComposer* serialized = reinterpret_cast<sSerializedPositionComposer*>(io_buffer);

		serialized->deadZone = i_toSerialize.GetDeadZone();
		serialized->softZone = i_toSerialize.GetSoftZone();
		serialized->targetOffset = i_toSerialize.GetTargetOffset();
		serialized->damping = i_toSerialize.GetDamping();
		serialized->screenPlaneNormalOverride = i_toSerialize.GetScreenPlaneNormalOverride();
		serialized->cameraUpDirectionOverride = i_toSerialize.GetCameraUpDirectionOverride();
	}

	template <>
	void Deserialize<Camera::cPositionComposer>(const char* i_buffer, Camera::cPositionComposer& io_toDeserialize)
	{
		const sSerializedPositionComposer* serialized = reinterpret_cast<const sSerializedPositionComposer*>(i_buffer);

		io_toDeserialize.SetDeadZone(serialized->deadZone);
		io_toDeserialize.SetSoftZone(serialized->softZone);
		io_toDeserialize.SetTargetOffset(serialized->targetOffset);
		io_toDeserialize.SetDamping(serialized->damping);
		io_toDeserialize.SetScreenPlaneNormalOverride(serialized->screenPlaneNormalOverride);
		io_toDeserialize.SetCameraUpDirectionOverride(serialized->cameraUpDirectionOverride);
	}
}