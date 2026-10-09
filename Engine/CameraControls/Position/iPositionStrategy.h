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
		virtual Math::sVector CalculatePosition(
			float i_deltaSeconds,
			const cTrackingCamera& i_trackingCamera,
			const Math::sVector& i_cameraPosition,
			const Math::cQuaternion& i_cameraOrientation,
			const Math::sVector& i_targetPosition,
			const Math::cQuaternion& i_targetOrientation) = 0;
	};
}