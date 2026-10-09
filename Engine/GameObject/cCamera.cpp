// Includes
//=========

#include "cCamera.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Graphics/Graphics.h>

// Interface
//==========

void eae6320::GameObject::cCamera::SetClippingPlanes( const float i_nearPlaneDistance, const float i_farPlaneDistance )
{
	EAE6320_ASSERTF( ( i_nearPlaneDistance > 0.0f ) && ( i_farPlaneDistance > i_nearPlaneDistance ),
		"The near plane must be in front of the camera and closer than the far plane" );
	m_nearPlaneDistance = i_nearPlaneDistance;
	m_farPlaneDistance = i_farPlaneDistance;
}

void eae6320::GameObject::cCamera::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate, const float i_aspectRatio ) const
{
	Graphics::SubmitCameraDataForANewFrame(
		m_rigidBodyState.PredictFuturePosition( i_elapsedSecondCount_sinceLastSimulationUpdate ),
		m_rigidBodyState.PredictFutureOrientation( i_elapsedSecondCount_sinceLastSimulationUpdate ),
		m_verticalFieldOfView_inRadians, i_aspectRatio, m_nearPlaneDistance, m_farPlaneDistance );
}
