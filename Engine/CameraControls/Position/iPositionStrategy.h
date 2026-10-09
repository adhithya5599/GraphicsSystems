#pragma once

/*
	This interface represents an abstract way for making
	a cTrackingCamera's position respond to a tracking target's
	position and orientation (as well as the camera's current position and orientation).
*/

namespace eae6320::Math
{
	struct sVector;
	class cQuaternion;
}

namespace eae6320::Camera
{
	class cTrackingCamera;

	class iPositionStrategy
	{
	public:
		// Strategies are owned (and deleted) through a std::unique_ptr<iPositionStrategy>,
		// so the destructor must be virtual:
		// otherwise deleting a derived strategy through the interface pointer is undefined behavior
		// (in practice the derived class's members, like a composer's child strategies, would never be destroyed)
		virtual ~iPositionStrategy() = default;

		virtual Math::sVector CalculatePosition(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) = 0;
	};
}