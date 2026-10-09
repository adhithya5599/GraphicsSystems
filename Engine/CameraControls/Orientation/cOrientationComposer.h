#pragma once

/*
	This class represents a camera orientation strategy, that will move the camera's rotation
	to keep the target in the frame (specifically within the dead/soft zones).
	This can be useful for a security camera.
*/

#include "iOrientationStrategy.h"
#include <Engine/Math/sVector.h>
#include <Engine/Math/sRectangle.h>
#include <Engine/Math/sVector2D.h>
#include "../Serialization.h"

namespace eae6320::Math
{
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cOrientationComposer : public iOrientationStrategy
	{
	public:
		cOrientationComposer() = default;

		// See cPositionComposer.h for details about dead zone, soft zone, and target offset

		const Math::sRectangle& GetDeadZone() const;
		void SetDeadZone(const Math::sRectangle& i_deadZone);
		void SetDeadZone(const Math::sVector2D& i_deadZone);

		const Math::sRectangle& GetSoftZone() const;
		void SetSoftZone(const Math::sRectangle& i_softZone);
		void SetSoftZone(const Math::sVector2D& i_softZone);

		const Math::sVector2D& GetTargetOffset() const;
		void SetTargetOffset(const Math::sVector2D& i_targetOffset);

		// By default, the camera's current up vector is used when calculating the look rotation
		// for looking at the target. Setting this to a value other than zero will use this value
		// instead in the calculation.
		// For example, setting this to the up vector will make the camera always upright.
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
		Math::sRectangle m_deadZone = Math::sRectangle(-0.25f, 0.25f, -0.25f, 0.25f);
		Math::sRectangle m_softZone = Math::sRectangle(-0.5f, 0.5f, -0.5f, 0.5f);
		Math::sVector2D m_targetOffset = Math::sVector2D::Zero;
		Math::sVector m_cameraUpDirectionOverride = Math::sVector();
		float m_damping = 0.f;
	};
}

namespace eae6320::Serialization
{
	struct sSerializedOrientationComposer
	{
		Math::sRectangle deadZone;
		Math::sRectangle softZone;
		Math::sVector2D targetOffset;
		Math::sVector cameraUpDirectionOverride;
		float damping;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cOrientationComposer>()
	{
		return sizeof(sSerializedOrientationComposer);
	}

	template <>
	void Serialize<Camera::cOrientationComposer>(const Camera::cOrientationComposer& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cOrientationComposer>(const char* i_buffer, Camera::cOrientationComposer& io_toDeserialize);
}