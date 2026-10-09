// Includes
//=========

#include "cShotCameraRig.h"

#include "GameplaySettings.h"

#include <Engine/CameraControls/Serialization.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Logging/Logging.h>

// Interface
//==========

// Initialize
//-----------

eae6320::cResult eae6320::OneShot::cShotCameraRig::Load( const float i_aspectRatio )
{
	auto result = Results::Success;

	m_aspectRatio = i_aspectRatio;
	if ( !( result = Serialization::LoadCameraFromFile( "data/Cameras/thirdPersonFollowCamera.camera", m_thirdPersonCamera ) ) )
	{
		Logging::OutputError( "The third-person camera couldn't be loaded" );
		return result;
	}
	if ( !( result = Serialization::LoadCameraFromFile( "data/Cameras/firstPersonCamera.camera", m_firstPersonCamera ) ) )
	{
		Logging::OutputError( "The first-person camera couldn't be loaded" );
		return result;
	}
	m_thirdPersonCamera.SetTrackingTarget( &m_thirdPersonTargetPosition, &m_targetOrientation );
	m_firstPersonCamera.SetTrackingTarget( &m_firstPersonTargetPosition, &m_targetOrientation );

	// The game starts aiming, looking through the third-person camera
	m_view = eView::ThirdPerson;
	m_transition.Start( Math::sVector(), Math::cQuaternion(), 0.0f );

	return result;
}

// Views
//------

void eae6320::OneShot::cShotCameraRig::SwitchTo( const eView i_view )
{
	if ( i_view == m_view )
	{
		return;
	}
	// The blend starts from what is being seen right now
	// (which might itself be partway through a previous blend)
	m_transition.Start( m_position, m_orientation, Settings::cameraTransitionDuration );
	m_verticalFieldOfView_transitionStart = m_verticalFieldOfView;
	m_view = i_view;
}

// Update
//-------

void eae6320::OneShot::cShotCameraRig::Track( const Math::sVector& i_thirdPersonSubject, const Math::sVector& i_firstPersonSubject,
	const Math::sVector& i_subjectVelocity )
{
	m_thirdPersonTargetPosition = i_thirdPersonSubject + Settings::thirdPersonTargetOffset;
	m_firstPersonTargetPosition = i_firstPersonSubject + Settings::firstPersonTargetOffset;
	m_subjectVelocity = i_subjectVelocity;

	if ( !m_hasTrackedBefore )
	{
		m_hasTrackedBefore = true;
		// The tracking cameras ease toward their targets (their "damping"),
		// so the very first time they are given a target they would start from the world origin
		// and visibly swoop into place.
		// Updating them with a long time step lets them settle before anything is rendered.
		// They are updated twice because a tracking camera calculates its new orientation
		// from where it was _before_ the update moved it,
		// so the first update would aim it from the origin rather than from where it ends up.
		constexpr float settleSecondCount = 10.0f;
		for ( auto i = 0; i < 2; ++i )
		{
			Update( settleSecondCount );
		}
	}
}

void eae6320::OneShot::cShotCameraRig::Update( const float i_elapsedSecondCount_sinceLastUpdate )
{
	m_thirdPersonCamera.Update( i_elapsedSecondCount_sinceLastUpdate );
	m_firstPersonCamera.Update( i_elapsedSecondCount_sinceLastUpdate );
	m_transition.Update( i_elapsedSecondCount_sinceLastUpdate );

	const auto& camera = GetCamera( m_view );
	m_transition.CalculateView( camera.GetPosition(), camera.GetOrientation(), m_position, m_orientation );
	// The two cameras have different fields of view, so that is blended too
	// (otherwise the view would visibly "zoom" at the start or end of the switch)
	const auto fieldOfView_destination = camera.GetFieldOfViewRadians().y;
	const auto weight = m_transition.GetBlendWeight();
	m_verticalFieldOfView = m_verticalFieldOfView_transitionStart + ( ( fieldOfView_destination - m_verticalFieldOfView_transitionStart ) * weight );
}

// Render
//-------

eae6320::Math::sVector eae6320::OneShot::cShotCameraRig::PredictFuturePosition( const float i_elapsedSecondCount_sinceLastSimulationUpdate ) const
{
	// The cameras only move during simulation updates,
	// but the things they follow are drawn extrapolated to the current moment.
	// Extrapolating the camera by the same amount keeps the arrow steady in front of the first-person camera
	// instead of shaking back and forth between updates.
	return m_position + ( m_subjectVelocity * i_elapsedSecondCount_sinceLastSimulationUpdate );
}

void eae6320::OneShot::cShotCameraRig::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate ) const
{
	// The near and far planes don't need to be blended (they don't visibly change anything),
	// so the destination camera's are used
	const auto& camera = GetCamera( m_view );
	Graphics::SubmitCameraDataForANewFrame( PredictFuturePosition( i_elapsedSecondCount_sinceLastSimulationUpdate ), m_orientation,
		m_verticalFieldOfView, m_aspectRatio, camera.GetZNearPlane(), camera.GetZFarPlane() );
}

// Implementation
//===============

const eae6320::Camera::cTrackingCamera& eae6320::OneShot::cShotCameraRig::GetCamera( const eView i_view ) const
{
	return ( i_view == eView::FirstPerson ) ? m_firstPersonCamera : m_thirdPersonCamera;
}
