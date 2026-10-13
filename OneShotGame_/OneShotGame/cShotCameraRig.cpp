// Includes
//=========

#include "cShotCameraRig.h"

#include "GameplaySettings.h"

#include <Engine/CameraControls/Serialization.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Logging/Logging.h>

#include <vector>

// Helper Declarations
//====================

namespace
{
	// Copies keyframes from the game's settings into a camera path
	template <size_t tKeyframeCount>
	void SetKeyframes( const std::array<eae6320::OneShot::Settings::sCameraKeyframe, tKeyframeCount>& i_keyframes,
		eae6320::Camera::cCameraPath& o_path );
}

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

	SetKeyframes( Settings::introKeyframes, m_introPath );
	SetKeyframes( Settings::resultKeyframes, m_resultPath );
	PlayIntro();

	return result;
}

// Views
//------

void eae6320::OneShot::cShotCameraRig::PlayIntro()
{
	m_view = eView::Intro;
	m_pathSecondCount = 0.0f;
	m_transition.Start( Math::sVector(), Math::cQuaternion(), 0.0f );
	CalculateUnblendedView( m_view, 0.0f, Math::sVector(), m_position, m_orientation, m_verticalFieldOfView );
}

void eae6320::OneShot::cShotCameraRig::PlayResultShot()
{
	// A switch with no transition time is a cut
	SwitchTo( eView::Result, 0.0f, true );
	m_pathSecondCount = 0.0f;
}

void eae6320::OneShot::cShotCameraRig::SwitchTo( const eView i_view, const float i_transitionDurationInSeconds, const bool i_shouldForce )
{
	if ( ( i_view == m_view ) && !i_shouldForce )
	{
		return;
	}
	// The blend starts from what is being seen right now
	// (which might itself be partway through a previous blend)
	m_transition.Start( m_position, m_orientation, i_transitionDurationInSeconds );
	m_verticalFieldOfView_transitionStart = m_verticalFieldOfView;
	m_view = i_view;
}

void eae6320::OneShot::cShotCameraRig::ToggleArrowView( const float i_transitionDurationInSeconds )
{
	SwitchTo( ( m_view == eView::FirstPerson ) ? eView::ThirdPerson : eView::FirstPerson, i_transitionDurationInSeconds );
}

// Update
//-------

void eae6320::OneShot::cShotCameraRig::Track( const Math::sVector& i_thirdPersonSubject, const Math::sVector& i_firstPersonSubject )
{
	m_thirdPersonTargetPosition = i_thirdPersonSubject + Settings::thirdPersonTargetOffset;
	m_firstPersonTargetPosition = i_firstPersonSubject + Settings::firstPersonTargetOffset;

	if ( !m_hasTrackedBefore )
	{
		m_hasTrackedBefore = true;
		// The very first time the cameras are given a target they would start from the world origin and visibly swoop into place
		SettleTrackingCameras();
	}
}

void eae6320::OneShot::cShotCameraRig::SettleTrackingCameras()
{
	// The tracking cameras ease toward their targets (their "damping").
	// Updating them with a long time step lets them settle all the way.
	// They are updated twice because a tracking camera calculates its new orientation
	// from where it was _before_ the update moved it,
	// so the first update would aim it from where it was rather than from where it ends up.
	constexpr float settleSecondCount = 10.0f;
	for ( auto i = 0; i < 2; ++i )
	{
		m_thirdPersonCamera.Update( settleSecondCount );
		m_firstPersonCamera.Update( settleSecondCount );
	}
}

void eae6320::OneShot::cShotCameraRig::Update( const float i_elapsedSecondCount_sinceLastUpdate )
{
	m_thirdPersonCamera.Update( i_elapsedSecondCount_sinceLastUpdate );
	m_firstPersonCamera.Update( i_elapsedSecondCount_sinceLastUpdate );
	m_transition.Update( i_elapsedSecondCount_sinceLastUpdate );
	if ( ( m_view == eView::Intro ) || ( m_view == eView::Result ) )
	{
		m_pathSecondCount += i_elapsedSecondCount_sinceLastUpdate;
		if ( ( m_view == eView::Intro ) && ( m_pathSecondCount >= m_introPath.GetDuration() ) )
		{
			// The opening shot's last keyframe is the aiming camera's view, so there's nothing to blend
			SwitchTo( eView::ThirdPerson, 0.0f );
		}
		// (The result shot holds its last keyframe, on the big screen, until the game switches to another view)
	}
	// Remember the current view so that a new transition can start from it
	CalculateView( 0.0f, Math::sVector(), m_position, m_orientation, m_verticalFieldOfView );
}

// Render
//-------

void eae6320::OneShot::cShotCameraRig::CalculateView( const float i_elapsedSecondCount_sinceLastSimulationUpdate, const Math::sVector& i_subjectVelocity,
	Math::sVector& o_position, Math::cQuaternion& o_orientation, float& o_verticalFieldOfView ) const
{
	Math::sVector position;
	Math::cQuaternion orientation;
	float verticalFieldOfView;
	CalculateUnblendedView( m_view, i_elapsedSecondCount_sinceLastSimulationUpdate, i_subjectVelocity, position, orientation, verticalFieldOfView );
	// While switching, blend from where the switch started (the blend is also extrapolated so that it is smooth)
	m_transition.CalculateView( position, orientation, o_position, o_orientation, i_elapsedSecondCount_sinceLastSimulationUpdate );
	const auto weight = m_transition.GetBlendWeight( i_elapsedSecondCount_sinceLastSimulationUpdate );
	o_verticalFieldOfView = m_verticalFieldOfView_transitionStart + ( ( verticalFieldOfView - m_verticalFieldOfView_transitionStart ) * weight );
}

void eae6320::OneShot::cShotCameraRig::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate, const Math::sVector& i_subjectVelocity ) const
{
	Math::sVector position;
	Math::cQuaternion orientation;
	float verticalFieldOfView;
	CalculateView( i_elapsedSecondCount_sinceLastSimulationUpdate, i_subjectVelocity, position, orientation, verticalFieldOfView );
	// The near and far planes don't need to be blended (they don't visibly change anything);
	// both cameras use the same ones
	const auto& camera = GetTrackingCamera( eView::ThirdPerson );
	Graphics::SubmitCameraDataForANewFrame( position, orientation, verticalFieldOfView, m_aspectRatio, camera.GetZNearPlane(), camera.GetZFarPlane() );
}

// Implementation
//===============

void eae6320::OneShot::cShotCameraRig::CalculateUnblendedView( const eView i_view, const float i_extraSeconds, const Math::sVector& i_subjectVelocity,
	Math::sVector& o_position, Math::cQuaternion& o_orientation, float& o_verticalFieldOfView ) const
{
	if ( ( i_view == eView::Intro ) || ( i_view == eView::Result ) )
	{
		// The scripted shots move by time, so they are extrapolated by evaluating the path a little later
		const auto& path = ( i_view == eView::Intro ) ? m_introPath : m_resultPath;
		path.CalculateView( m_pathSecondCount + i_extraSeconds, o_position, o_orientation );
		o_verticalFieldOfView = m_thirdPersonCamera.GetFieldOfViewRadians().y;
	}
	else
	{
		const auto& camera = GetTrackingCamera( i_view );
		// The tracking cameras only move during simulation updates,
		// so they are extrapolated by the velocity of what they follow (like the things being followed are)
		o_position = camera.GetPosition() + ( i_subjectVelocity * i_extraSeconds );
		o_orientation = camera.GetOrientation();
		o_verticalFieldOfView = camera.GetFieldOfViewRadians().y;
	}
}

const eae6320::Camera::cTrackingCamera& eae6320::OneShot::cShotCameraRig::GetTrackingCamera( const eView i_view ) const
{
	return ( i_view == eView::FirstPerson ) ? m_firstPersonCamera : m_thirdPersonCamera;
}

// Helper Definitions
//===================

namespace
{
	template <size_t tKeyframeCount>
	void SetKeyframes( const std::array<eae6320::OneShot::Settings::sCameraKeyframe, tKeyframeCount>& i_keyframes,
		eae6320::Camera::cCameraPath& o_path )
	{
		std::vector<eae6320::Camera::cCameraPath::sKeyframe> keyframes;
		for ( const auto& keyframe : i_keyframes )
		{
			keyframes.push_back( eae6320::Camera::cCameraPath::sKeyframe{ keyframe.time, keyframe.position, keyframe.lookAtPosition } );
		}
		o_path.SetKeyframes( keyframes );
	}
}
