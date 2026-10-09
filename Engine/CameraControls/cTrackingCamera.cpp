#include "cTrackingCamera.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>
#include "Position/iPositionStrategy.h"
#include "Orientation/iOrientationStrategy.h"

namespace eae6320::Camera
{
	// Getters/Setters
	//-----------

	const Math::sVector& cTrackingCamera::GetPosition() const
	{
		return m_position;
	}

	void cTrackingCamera::SetPosition(const Math::sVector& i_position)
	{
		m_position = i_position;
	}

	const Math::cQuaternion& cTrackingCamera::GetOrientation() const
	{
		return m_orientation;
	}

	void cTrackingCamera::SetOrientation(const Math::cQuaternion& i_orientation)
	{
		m_orientation = i_orientation;
	}

	const iPositionStrategy* cTrackingCamera::GetPositionStrategy() const
	{
		return m_positionStrategy.get();
	}

	iPositionStrategy* cTrackingCamera::GetPositionStrategy()
	{
		return m_positionStrategy.get();
	}

	void cTrackingCamera::SetPositionStrategy(iPositionStrategy* i_positionStrategy)
	{
		m_positionStrategy = std::unique_ptr<iPositionStrategy>(i_positionStrategy);
	}

	void cTrackingCamera::SetPositionStrategy(std::unique_ptr<iPositionStrategy> i_positionStrategy)
	{
		m_positionStrategy = std::move(i_positionStrategy);
	}

	std::unique_ptr<iPositionStrategy> cTrackingCamera::TakePositionStrategy()
	{
		return std::move(m_positionStrategy);
	}

	const iOrientationStrategy* cTrackingCamera::GetOrientationStrategy() const
	{
		return m_orientationStrategy.get();
	}

	iOrientationStrategy* cTrackingCamera::GetOrientationStrategy()
	{
		return m_orientationStrategy.get();
	}

	void cTrackingCamera::SetOrientationStrategy(iOrientationStrategy* i_orientationStrategy)
	{
		m_orientationStrategy = std::unique_ptr<iOrientationStrategy>(i_orientationStrategy);
	}

	void cTrackingCamera::SetOrientationStrategy(std::unique_ptr<iOrientationStrategy> i_orientationStrategy)
	{
		m_orientationStrategy = std::move(i_orientationStrategy);
	}

	std::unique_ptr<iOrientationStrategy> cTrackingCamera::TakeOrientationStrategy()
	{
		return std::move(m_orientationStrategy);
	}

	const Math::sVector2D& cTrackingCamera::GetFieldOfViewRadians() const
	{
		return m_fieldOfViewRadians;
	}

	void cTrackingCamera::SetFieldOfViewRadians(const Math::sVector2D& i_fieldOfViewRadians)
	{
		m_fieldOfViewRadians = i_fieldOfViewRadians;
	}

	float cTrackingCamera::GetZNearPlane() const
	{
		return m_zNearPlane;
	}

	void cTrackingCamera::SetZNearPlane(float i_zNearPlane)
	{
		m_zNearPlane = i_zNearPlane;
	}

	float cTrackingCamera::GetZFarPlane() const
	{
		return m_zFarPlane;
	}

	void cTrackingCamera::SetZFarPlane(float i_zFarPlane)
	{
		m_zFarPlane = i_zFarPlane;
	}

	void cTrackingCamera::SetTrackingTarget(
		const Math::sVector* i_targetPosition, const Math::cQuaternion* i_targetOrientation)
	{
		m_targetPosition = i_targetPosition;
		m_targetOrientation = i_targetOrientation;
	}

	// Update/Submit
	//-----------

	void cTrackingCamera::Update(float i_deltaSeconds)
	{
		// A camera can't be updated until it has been loaded (which gives it its strategies)
		// and has been told what to track
		if (!m_positionStrategy || !m_orientationStrategy || !m_targetPosition || !m_targetOrientation)
		{
			EAE6320_ASSERTF(false, "A tracking camera needs position/orientation strategies and a target before it can be updated");
			return;
		}

		Math::sVector newPosition = m_positionStrategy->CalculatePosition(
			i_deltaSeconds, *this, GetPosition(), GetOrientation(), *m_targetPosition, *m_targetOrientation);
		Math::cQuaternion newOrientation = m_orientationStrategy->CalculateOrientation(
			i_deltaSeconds, *this, GetPosition(), GetOrientation(), *m_targetPosition, *m_targetOrientation);

		SetPosition(newPosition);
		SetOrientation(newOrientation);
	}

	void cTrackingCamera::SubmitRenderData(const Math::sVector& i_cameraPosition, const Math::cQuaternion& i_cameraOrientation, float i_aspectRatio)
	{

		// TODO: replace this with your own way of submitting render data to the graphics library
		Graphics::SubmitCameraDataForANewFrame(
			i_cameraPosition,
			i_cameraOrientation,
			GetFieldOfViewRadians().y,
			i_aspectRatio,
			GetZNearPlane(),
			GetZFarPlane()
		);
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cTrackingCamera>(const Camera::cTrackingCamera& i_toSerialize, char* io_buffer)
	{
		sSerializedTrackingCamera* serialized = reinterpret_cast<sSerializedTrackingCamera*>(io_buffer);

		serialized->fieldOfViewRadians = i_toSerialize.GetFieldOfViewRadians();
		serialized->zNearPlane = i_toSerialize.GetZNearPlane();
		serialized->zFarPlane = i_toSerialize.GetZFarPlane();
	}

	template <>
	void Deserialize<Camera::cTrackingCamera>(const char* i_buffer, Camera::cTrackingCamera& io_toDeserialize)
	{
		const sSerializedTrackingCamera* serialized = reinterpret_cast<const sSerializedTrackingCamera*>(i_buffer);

		io_toDeserialize.SetFieldOfViewRadians(serialized->fieldOfViewRadians);
		io_toDeserialize.SetZNearPlane(serialized->zNearPlane);
		io_toDeserialize.SetZFarPlane(serialized->zFarPlane);
	}
}