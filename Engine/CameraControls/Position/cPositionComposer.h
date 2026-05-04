#pragma once

/*
	This class represents a camera positioning strategy, that will move the camera's position
	to keep the target in the frame (specifically within the dead/soft zones).
	This can be useful for top-down cameras.
*/

#include <Engine/Math/sVector.h>
#include <Engine/Math/sVector2D.h>
#include <Engine/Math/sRectangle.h>
#include "iPositionStrategy.h"
#include "../Serialization.h"

namespace eae6320::Math
{
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cPositionComposer : public iPositionStrategy
	{
	public:
		cPositionComposer() = default;

		// m_deadZone, m_softZone, and m_targetOffset are in screen coordinates,
		// where (-1, -1) is the bottom-left of the screen, and (1, 1) is the top-right of the screen

		// The dead zone is the area on the screen, where if the target is in it, the camera won't move;
		// if the target is outside the dead zone but in the soft zone, the camera will have damped movement
		// to get the target back into the dead zone
		const Math::sRectangle& GetDeadZone() const;
		void SetDeadZone(const Math::sRectangle& i_deadZone);
		void SetDeadZone(const Math::sVector2D& i_deadZone);

		// The soft zone is the area on the screen, where if the target is not in it,
		// the camera will snap so that the target is in the soft zone.
		const Math::sRectangle& GetSoftZone() const;
		void SetSoftZone(const Math::sRectangle& i_softZone);
		void SetSoftZone(const Math::sVector2D& i_softZone);

		const Math::sVector2D& GetTargetOffset() const;
		void SetTargetOffset(const Math::sVector2D& i_damping);

		// The y-component of damping is parallel to the camera's current up vector
		// (or m_cameraUpDirectionOverride if it's overriding), while the x-component is perpendicular damping.
		const Math::sVector2D& GetDamping() const;
		void SetDamping(const Math::sVector2D& i_targetOffset);

		// By default, the camera will move along the plane (containing itself) normal to its current forward vector.
		// If this is set to something other than the zero vector, it will replace the plane normal.
		// For example, setting this to the up vector makes the camera not change its y-position.
		const Math::sVector& GetScreenPlaneNormalOverride() const;
		void SetScreenPlaneNormalOverride(const Math::sVector& i_screenPlaneNormalOverride);

		bool IsScreenPlaneNormalOverridden() const;
		void ResetScreenPlaneNormalOverride();

		// Camera up direction override is currently only used for damping calculations
		// (if set to something other than the zero vector)
		const Math::sVector& GetCameraUpDirectionOverride() const;
		void SetCameraUpDirectionOverride(const Math::sVector& i_cameraUpDirectionOverride);

		bool IsCameraUpDirectionOverridden() const;
		void ResetCameraUpDirectionOverride();

		Math::sVector CalculatePosition(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) override;

	private:
		Math::sRectangle m_deadZone = Math::sRectangle(-0.25f, 0.25f, -0.25f, 0.25f);
		Math::sRectangle m_softZone = Math::sRectangle(-0.5f, 0.5f, -0.5f, 0.5f);
		Math::sVector2D m_targetOffset = Math::sVector2D::Zero;

		Math::sVector2D m_damping = Math::sVector2D::Zero;

		Math::sVector m_screenPlaneNormalOverride = Math::sVector();
		Math::sVector m_cameraUpDirectionOverride = Math::sVector();
	};
}

namespace eae6320::Serialization
{
	struct sSerializedPositionComposer
	{
		Math::sRectangle deadZone;
		Math::sRectangle softZone;
		Math::sVector2D targetOffset;
		Math::sVector2D damping;
		Math::sVector screenPlaneNormalOverride;
		Math::sVector cameraUpDirectionOverride;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cPositionComposer>()
	{
		return sizeof(sSerializedPositionComposer);
	}

	template <>
	void Serialize<Camera::cPositionComposer>(const Camera::cPositionComposer& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cPositionComposer>(const char* i_buffer, Camera::cPositionComposer& io_toDeserialize);
}