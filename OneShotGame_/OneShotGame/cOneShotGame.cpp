// Includes
//=========

#include "cOneShotGame.h"

#include "GameplaySettings.h"
#include "OneShotLock.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Physics/Collision3D.h>
#include <Engine/UserInput/UserInput.h>
#include <Engine/UserOutput/UserOutput.h>

// Helper Declarations
//====================

namespace
{
	// Returns -1, 0, or 1 depending on which of the two keys is held
	float GetAxis( const uint_fast8_t i_negativeKey, const uint_fast8_t i_positiveKey );
}

// Inherited Implementation
//=========================

// Run
//----

void eae6320::cOneShotGame::UpdateBasedOnInput()
{
	if ( m_isExiting )
	{
		return;
	}

	// Every tracked key is checked every frame, before anything can return early,
	// because each check is what lets the tracker notice the next press
	const auto wasPausePressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::P );
	const auto wasSlowMotionPressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::O );
	const auto wasLaunchPressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::Space );
	const auto wasViewSwitchPressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::F );

	if ( UserInput::IsKeyPressed( UserInput::KeyCodes::Escape ) )
	{
		ExitGame();
		return;
	}

	switch ( m_gameState )
	{
	case eGameState::AlreadyPlayed:
		{
			// (The player was already told why in Initialize())
			ExitGame();
		}
		return;
	case eGameState::ShowingResult:
		{
			// The result is held on screen for a moment (so the player sees where the arrow ended up)
			// and then the game closes
			if ( m_resultSecondCount >= OneShot::Settings::resultDisplayDuration )
			{
				ExitGame();
			}
		}
		return;
	default:
		break;
	}

	// Pausing and slow motion change how fast time passes,
	// which is application state rather than simulation state, so they take effect immediately
	// (and they must be handled here rather than in UpdateSimulationBasedOnInput(),
	// because while paused there are no simulation updates, so there would be no way to unpause)
	if ( wasPausePressed )
	{
		m_isPaused = !m_isPaused;
		UpdateSimulationRate();
	}
	if ( wasSlowMotionPressed )
	{
		m_isSlowMotionOn = !m_isSlowMotionOn;
		UpdateSimulationRate();
	}
	if ( !m_isPaused )
	{
		// Launching and switching cameras do change the simulation,
		// so they are only requested here and actually happen in the next simulation update
		if ( wasLaunchPressed && ( m_gameState == eGameState::Aiming ) )
		{
			m_isLaunchRequested = true;
		}
		if ( wasViewSwitchPressed && ( m_gameState == eGameState::Flying ) )
		{
			m_isViewSwitchRequested = true;
		}
	}
}

void eae6320::cOneShotGame::UpdateSimulationBasedOnInput()
{
	if ( m_isLaunchRequested )
	{
		m_isLaunchRequested = false;
		if ( m_gameState == eGameState::Aiming )
		{
			LaunchArrow();
		}
	}
	if ( m_isViewSwitchRequested )
	{
		m_isViewSwitchRequested = false;
		if ( m_gameState == eGameState::Flying )
		{
			m_cameraRig.ToggleView();
		}
	}

	if ( m_gameState == eGameState::Flying )
	{
		// WASD steer and Shift boosts
		// (boosting is disabled in slow motion so that slow motion can't be used to cheat)
		const auto horizontal = GetAxis( UserInput::KeyCodes::A, UserInput::KeyCodes::D );
		const auto vertical = GetAxis( UserInput::KeyCodes::S, UserInput::KeyCodes::W );
		const auto isBoosting = UserInput::IsKeyPressed( UserInput::KeyCodes::Shift ) && !m_isSlowMotionOn;
		m_arrow.Steer( horizontal, vertical, isBoosting );
	}
}

void eae6320::cOneShotGame::UpdateSimulationBasedOnTime( const float i_elapsedSecondCount_sinceLastUpdate )
{

	if ( m_gameState == eGameState::ShowingResult )
	{
		// Everything stays frozen where the shot ended
		m_resultSecondCount += i_elapsedSecondCount_sinceLastUpdate;
	}
	else
	{
		m_obstacleCourse.Update( i_elapsedSecondCount_sinceLastUpdate );
		m_arrow.Update( i_elapsedSecondCount_sinceLastUpdate );
		if ( m_gameState == eGameState::Flying )
		{
			CheckForShotResult();
		}
	}

	// The third-person camera watches the bow until the arrow is launched and then follows the arrow
	const auto& thirdPersonSubject = ( m_gameState == eGameState::Aiming ) ? OneShot::Settings::bowPosition : m_arrow.GetPosition();
	m_cameraRig.Track( thirdPersonSubject, m_arrow.GetPosition(), m_arrow.GetVelocity() );
	m_cameraRig.Update( i_elapsedSecondCount_sinceLastUpdate );
}

void eae6320::cOneShotGame::SubmitDataToBeRendered( const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	if ( m_gameState == eGameState::AlreadyPlayed )
	{
		// Nothing was loaded
		Graphics::SubmitBackgroundColorForANewFrame( 0.0f, 0.0f, 0.0f );
		return;
	}

	m_cameraRig.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	if ( m_gameState == eGameState::ShowingResult )
	{
		// Until the game has a UI, the result is shown by replacing the sky with a solid color
		// (green for a hit, red for an obstacle, and gray for a miss)
		switch ( m_shotResult )
		{
		case eShotResult::HitTarget: Graphics::SubmitBackgroundColorForANewFrame( 0.15f, 0.65f, 0.25f ); break;
		case eShotResult::HitObstacle: Graphics::SubmitBackgroundColorForANewFrame( 0.70f, 0.15f, 0.12f ); break;
		case eShotResult::Missed: Graphics::SubmitBackgroundColorForANewFrame( 0.35f, 0.35f, 0.38f ); break;
		}
	}
	else
	{
		Graphics::SubmitBackgroundColorForANewFrame( 0.0f, 0.0f, 0.0f );
		// The sky is always centered on the camera so that the camera can never get close to its edges
		// (which makes it look infinitely far away)
		m_skybox.SubmitToBeRendered( Math::cMatrix_transformation( Math::cQuaternion(),
			m_cameraRig.PredictFuturePosition( i_elapsedSecondCount_sinceLastSimulationUpdate ) ) );
	}

	m_ground.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	m_target.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	m_bow.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	m_obstacleCourse.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	m_arrow.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::cOneShotGame::Initialize()
{
	auto result = Results::Success;

	if ( OneShot::Lock::HasShotBeenTaken() )
	{
		// The game still initializes successfully, but nothing is loaded and it closes on its first update.
		// The message box must be shown here rather than later:
		// Initialize() runs on the main thread, but the update functions run on the application thread,
		// and a message box owned by the main window would deadlock there
		// (it needs the main thread to respond, but the main thread is waiting for the application thread to submit a frame).
		UserOutput::Print( "You only get one shot, and it has already been taken on this computer." );
		m_gameState = eGameState::AlreadyPlayed;
		return result;
	}

	if ( !( result = m_assets.Load() ) )
	{
		EAE6320_ASSERTF( false, "The game's assets couldn't be loaded" );
		return result;
	}
	{
		float aspectRatio = 1.0f;
		uint16_t width = 0, height = 0;
		if ( GetCurrentResolution( width, height ) && ( height > 0 ) )
		{
			aspectRatio = static_cast<float>( width ) / static_cast<float>( height );
		}
		if ( !( result = m_cameraRig.Load( aspectRatio ) ) )
		{
			EAE6320_ASSERTF( false, "The game's cameras couldn't be loaded" );
			return result;
		}
	}

	// The objects that don't move
	{
		struct sStaticObject
		{
			GameObject::cGameObject& gameObject;
			Graphics::cMesh* mesh;
			Graphics::cEffect* effect;
			Texture::cTexture* texture;
			Math::sVector position;
		};
		const sStaticObject staticObjects[] =
		{
			{ m_bow, m_assets.bowMesh, m_assets.litEffect, m_assets.bowTexture, OneShot::Settings::bowPosition },
			{ m_ground, m_assets.groundMesh, m_assets.litEffect, m_assets.groundTexture, Math::sVector() },
			{ m_target, m_assets.targetMesh, m_assets.litEffect, m_assets.targetTexture, OneShot::Settings::targetPosition },
			// The sky isn't lit: it should look the same in every direction regardless of where the lights are
			{ m_skybox, m_assets.skyboxMesh, m_assets.unlitEffect, m_assets.skyboxTexture, Math::sVector() },
		};
		for ( const auto& staticObject : staticObjects )
		{
			staticObject.gameObject.SetMesh( staticObject.mesh );
			staticObject.gameObject.SetEffect( staticObject.effect );
			staticObject.gameObject.SetTexture( staticObject.texture );
			staticObject.gameObject.GetRigidBodyState().position = staticObject.position;
		}
	}
	m_arrow.Initialize( m_assets );
	m_obstacleCourse.Initialize( m_assets );
	m_cameraRig.Track( OneShot::Settings::bowPosition, m_arrow.GetPosition(), Math::sVector() );

	m_gameState = eGameState::Aiming;
	Logging::OutputMessage( "One Shot controls: Space launches, WASD steer, Shift boosts, F switches camera, O slow motion, P pause" );

	return result;
}

eae6320::cResult eae6320::cOneShotGame::CleanUp()
{
	// Everything must be released here rather than in destructors
	// because this game object is destroyed after the Graphics system has shut down
	m_arrow.ReleaseAssets();
	m_obstacleCourse.ReleaseAssets();
	for ( auto* const gameObject : { &m_bow, &m_ground, &m_target, &m_skybox } )
	{
		gameObject->ReleaseAssets();
	}
	m_assets.Release();

	return Results::Success;
}

// Implementation
//===============

void eae6320::cOneShotGame::LaunchArrow()
{
	m_arrow.Launch();
	m_gameState = eGameState::Flying;
	// The shot counts as soon as the arrow leaves the bow
	// (recording it only at the end would let a player close the game mid-flight and try again)
	OneShot::Lock::RecordThatShotWasTaken();
	m_cameraRig.SwitchTo( OneShot::cShotCameraRig::eView::FirstPerson );
}

void eae6320::cOneShotGame::CheckForShotResult()
{
	float timeOfImpact = 0.0f;
	if ( m_obstacleCourse.IsHitBy( m_arrow, timeOfImpact ) )
	{
		// The arrow stops where it touched the obstacle (rather than wherever the update ended, which could be inside it)
		m_arrow.StopPartwayThroughLastUpdate( timeOfImpact );
		EndShot( eShotResult::HitObstacle );
		return;
	}

	// The target is thinner than the distance the arrow can travel in one simulation update
	// (at 15 updates per second a boosted arrow moves about 0.67 units, and the target is about 0.43 units thick),
	// so the tip is swept along its whole path: checking only where it ended up could skip right through the target
	const Physics::sAabb3D targetCollider{
		OneShot::Settings::targetPosition + OneShot::Settings::targetColliderCenter_local,
		OneShot::Settings::targetColliderHalfExtents };
	if ( Physics::Collision3D::SweepSphereAgainstAabb( m_arrow.GetTipAtStartOfLastUpdate(), m_arrow.GetTipMovementDuringLastUpdate(),
		targetCollider, timeOfImpact ) )
	{
		// The arrow stops where the tip touched the target (rather than where the update ended, which could be inside or behind it)
		m_arrow.StopPartwayThroughLastUpdate( timeOfImpact );
		EndShot( eShotResult::HitTarget );
		return;
	}

	if ( m_arrow.GetTipPosition().z < ( OneShot::Settings::targetPosition.z - OneShot::Settings::missDistancePastTarget ) )
	{
		EndShot( eShotResult::Missed );
	}
}

void eae6320::cOneShotGame::EndShot( const eShotResult i_result )
{
	m_arrow.Stop();
	m_shotResult = i_result;
	m_gameState = eGameState::ShowingResult;
	m_resultSecondCount = 0.0f;
	// The result is shown at normal speed
	m_isPaused = false;
	m_isSlowMotionOn = false;
	UpdateSimulationRate();
	// Pull back to the third-person view to show where the arrow ended up
	m_cameraRig.SwitchTo( OneShot::cShotCameraRig::eView::ThirdPerson );

	const char* const resultDescription =
		( i_result == eShotResult::HitTarget ) ? "hit the target" : ( ( i_result == eShotResult::HitObstacle ) ? "hit an obstacle" : "missed" );
	Logging::OutputMessage( "The shot %s", resultDescription );
}

void eae6320::cOneShotGame::UpdateSimulationRate()
{
	// The application's simulation rate scales how much simulation time passes per real second:
	// 0 stops the simulation completely, and anything between 0 and 1 is slow motion.
	// Everything that uses simulation time (movement, the cameras, even the extrapolation used for rendering)
	// slows down together, which wouldn't be true if the game scaled the elapsed time itself.
	SetSimulationRate( m_isPaused ? 0.0f : ( m_isSlowMotionOn ? OneShot::Settings::slowMotionRate : 1.0f ) );
}

void eae6320::cOneShotGame::ExitGame()
{
	m_isExiting = true;
	const auto result = Exit( EXIT_SUCCESS );
	EAE6320_ASSERT( result );
}

// Helper Definitions
//===================

namespace
{
	float GetAxis( const uint_fast8_t i_negativeKey, const uint_fast8_t i_positiveKey )
	{
		float axis = 0.0f;
		if ( eae6320::UserInput::IsKeyPressed( i_negativeKey ) )
		{
			axis -= 1.0f;
		}
		if ( eae6320::UserInput::IsKeyPressed( i_positiveKey ) )
		{
			axis += 1.0f;
		}
		return axis;
	}
}
