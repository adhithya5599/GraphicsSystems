#pragma once

/*
	This interface represents an abstract way for making
	a cTrackingCamera's orientation respond to a tracking target's
	position and orientation (as well as the camera's current position and orientation).
*/

#include <memory>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector2D.h>
#include <Engine/Math/sVector.h>
#include "Serialization.h"

namespace eae6320::Camera
{
	class iPositionStrategy;
	class iOrientationStrategy;

	class cTrackingCamera
	{
	public:
		// Initialize
		//-----------

		cTrackingCamera() = default;

		// Getters/Setters
		//-----------
		
		// The camera's current position
		const Math::sVector& GetPosition() const;
		void SetPosition(const Math::sVector& i_position);

		// The camera's current orientation
		const Math::cQuaternion& GetOrientation() const;
		void SetOrientation(const Math::cQuaternion& i_orientation);

		// The abstract strategy that this tracking camera will use to determine its position.
		// Note that this cTrackingCamera takes ownership (e.g. responsibilty for freeing)
		// of the iPositionStrategy. You can borrow a reference to it using the getter.
		const iPositionStrategy* GetPositionStrategy() const;
		iPositionStrategy* GetPositionStrategy();
		void SetPositionStrategy(iPositionStrategy* i_positionStrategy);
		void SetPositionStrategy(std::unique_ptr<iPositionStrategy> i_positionStrategy);

		// Takes back ownership of this camera's position strategy.
		// This means this camera will no longer have a position strategy,
		// so you'll need to give it a new one before Update is called.
		std::unique_ptr<iPositionStrategy> TakePositionStrategy();

		// The abstract strategy that this tracking camera will use to determine its orientation.
		// Note that this cTrackingCamera takes ownership (e.g. responsibilty for freeing)
		// of the iOrientationStrategy. You can borrow a reference to it using the getter.
		const iOrientationStrategy* GetOrientationStrategy() const;
		iOrientationStrategy* GetOrientationStrategy();
		void SetOrientationStrategy(iOrientationStrategy* i_orientationStrategy);
		void SetOrientationStrategy(std::unique_ptr<iOrientationStrategy> i_orientationStrategy);

		// Takes back ownership of this camera's orientation strategy.
		// This means this camera will no longer have a orientation strategy,
		// so you'll need to give it a new one before Update is called.
		std::unique_ptr<iOrientationStrategy> TakeOrientationStrategy();

		// The horizontal and vertical fields of view for the camera.
		// If you set one axis to a given value,
		// you should set the other axis based on that value and the screen's aspect ratio.
		const Math::sVector2D& GetFieldOfViewRadians() const;
		void SetFieldOfViewRadians(const Math::sVector2D& i_fieldOfViewRadians);

		// Objects closer than the near plane will not be rendered for this camra
		float GetZNearPlane() const;
		void SetZNearPlane(float i_zNearPlane);

		// Objects past the far plane will not be rendered for this camera
		float GetZFarPlane() const;
		void SetZFarPlane(float i_zFarPlane);

		// Pointers to the position and orientation of the tracking target.
		// Assumes the memory address of the target's position/orientation doesn't change.
		// If you move where the position/orientation is in memory, you'll have to set the tracking target again.
		void SetTrackingTarget(const Math::sVector* i_targetPosition, const Math::cQuaternion* i_targetOrientation);

		// Update/Submit
		//-----------

		// Updates this camera's position and orientation, based on the strategies passed into this cTrackingCamera.
		// Meant to be called when the application state is updated (e.g. iApplication::UpdateBasedOnTime).
		void Update(float i_deltaSeconds);

		// Submits render data to this solution's graphics library.
		// If you use this library in another solution, you'll need to re-implement this method
		// to submit render data into your respective graphics library.
		void SubmitRenderData(const Math::sVector& i_cameraPosition, const Math::cQuaternion& i_cameraOrientation, float i_aspectRatio);

	private:
		// Camera position/orientation
		//-----------
		Math::sVector m_position = Math::sVector();
		Math::cQuaternion m_orientation = Math::cQuaternion();
		
		// Camera positioning/orientating
		//-----------

		std::unique_ptr<iPositionStrategy> m_positionStrategy;
		std::unique_ptr<iOrientationStrategy> m_orientationStrategy;

		// Target
		//-----------

		const Math::sVector* m_targetPosition = nullptr;
		const Math::cQuaternion* m_targetOrientation = nullptr;

		// Camera data
		//-----------

		Math::sVector2D m_fieldOfViewRadians = Math::sVector2D();
		float m_zNearPlane = 0.1f;
		float m_zFarPlane = 100.0f;
	};
}

namespace eae6320::Serialization
{
	struct sSerializedTrackingCamera
	{
		Math::sVector2D fieldOfViewRadians;
		float zNearPlane;
		float zFarPlane;
	};

	template <>
	constexpr size_t GetSerializedSize<Camera::cTrackingCamera>()
	{
		return sizeof(sSerializedTrackingCamera);
	}

	template <>
	void Serialize<Camera::cTrackingCamera>(const Camera::cTrackingCamera& i_toSerialize, char* io_buffer);

	template <>
	void Deserialize<Camera::cTrackingCamera>(const char* i_buffer, Camera::cTrackingCamera& io_toDeserialize);
}