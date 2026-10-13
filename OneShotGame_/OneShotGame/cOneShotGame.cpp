// Includes
//=========

#include "cOneShotGame.h"

#include "GameplaySettings.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Audio/cSound.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/UserInput/UserInput.h>

// Helper Declarations
//====================

namespace
{
	// Returns -1, 0, or 1 depending on which of the two keys is held
	float GetAxis( const uint_fast8_t i_negativeKey, const uint_fast8_t i_positiveKey );

	// The color behind the sky dome (it shows only at the very bottom edge of the dome)
	constexpr float s_horizonColor[] = { 0.80f, 0.87f, 0.93f };
}

// Inherited Implementation
//=========================

// Run
//----

void eae6320::cOneShotGame::UpdateBasedOnInput()
{
	// Every tracked key is checked every frame, before anything can return early,
	// because each check is what lets the tracker notice the next press
	const auto wasPausePressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::P );
	const auto wasSlowMotionPressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::O );
	const auto wasSpacePressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::Space );
	const auto wasViewSwitchPressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::F );
	const auto wasIntroReplayPressed = m_keyPresses.WasKeyJustPressed( UserInput::KeyCodes::R );

	if ( UserInput::IsKeyPressed( UserInput::KeyCodes::Escape ) )
	{
		const auto result = Exit( EXIT_SUCCESS );
		EAE6320_ASSERT( result );
		return;
	}

	// Pausing and slow motion change how fast time passes,
	// which is application state rather than simulation state, so they take effect immediately
	// (and they must be handled here rather than in UpdateSimulationBasedOnInput(),
	// because while paused there are no simulation updates, so there would be no way to unpause)
	if ( wasPausePressed )
	{
		m_isPaused = !m_isPaused;
		UpdateSimulationRate();
		// Sounds pause too (otherwise a cheer would carry on over a frozen game)
		Audio::SetIsPaused( m_isPaused );
	}
	if ( wasSlowMotionPressed && ( m_gameState == eGameState::Flying ) )
	{
		m_isSlowMotionOn = !m_isSlowMotionOn;
		UpdateSimulationRate();
	}
	if ( !m_isPaused )
	{
		// Everything else changes the simulation,
		// so it is only requested here and actually happens in the next simulation update.
		// Space does whatever makes sense right now: skip the opening shot, launch, or take another shot.
		if ( wasSpacePressed )
		{
			m_isSpaceRequested = true;
			m_gameState_whenSpaceWasPressed = m_gameState;
		}
		if ( wasViewSwitchPressed && ( m_gameState == eGameState::Flying ) )
		{
			m_isViewSwitchRequested = true;
		}
		if ( wasIntroReplayPressed && ( m_gameState == eGameState::Aiming ) )
		{
			m_isIntroReplayRequested = true;
		}
	}
}

void eae6320::cOneShotGame::UpdateSimulationBasedOnInput()
{
	if ( m_isSpaceRequested )
	{
		m_isSpaceRequested = false;
		// (A press is ignored if the state changed since it was pressed)
		switch ( ( m_gameState == m_gameState_whenSpaceWasPressed ) ? m_gameState : eGameState::Flying )
		{
		case eGameState::Intro:
			m_cameraRig.SwitchTo( OneShot::cShotCameraRig::eView::ThirdPerson, OneShot::Settings::introSkipTransitionDuration );
			StartAiming();
			StartFanfareFadeOut();
			break;
		case eGameState::Aiming:
			LaunchArrow();
			break;
		case eGameState::ShowingResult:
			{
				const auto minimumDuration = m_round.IsOver()
					? OneShot::Settings::roundOverMinimumDisplayDuration : OneShot::Settings::resultMinimumDisplayDuration;
				if ( m_resultSecondCount >= minimumDuration )
				{
					TakeAnotherShot();
				}
			}
			break;
		default:
			break;
		}
	}
	if ( m_isViewSwitchRequested )
	{
		m_isViewSwitchRequested = false;
		if ( m_gameState == eGameState::Flying )
		{
			m_cameraRig.ToggleArrowView( OneShot::Settings::cameraTransitionDuration );
		}
	}
	if ( m_isIntroReplayRequested )
	{
		m_isIntroReplayRequested = false;
		if ( m_gameState == eGameState::Aiming )
		{
			PlayIntro();
		}
	}

	if ( m_gameState == eGameState::Flying )
	{
		// WASD steer, Shift boosts (but not in slow motion, so slow motion can't be used to cheat),
		// and the wind pushes the arrow sideways
		const auto horizontal = GetAxis( UserInput::KeyCodes::A, UserInput::KeyCodes::D );
		const auto vertical = GetAxis( UserInput::KeyCodes::S, UserInput::KeyCodes::W );
		const auto isBoosting = UserInput::IsKeyPressed( UserInput::KeyCodes::Shift ) && !m_isSlowMotionOn;
		m_arrow.Steer( horizontal, vertical, isBoosting, m_wind.GetSpeed() * OneShot::Settings::windDriftFactor );
	}
}

void eae6320::cOneShotGame::UpdateSimulationBasedOnTime( const float i_elapsedSecondCount_sinceLastUpdate )
{
	// The wind and the obstacles keep moving in every state
	// (so the player can watch their patterns while aiming, and the result shows the obstacles still going)
	m_wind.Update( i_elapsedSecondCount_sinceLastUpdate );
	m_obstacleCourse.Update( i_elapsedSecondCount_sinceLastUpdate, m_arrow.GetTipPosition(), m_arrow.IsFlying() );
	m_arrow.Update( i_elapsedSecondCount_sinceLastUpdate );
	m_crowd.Update( i_elapsedSecondCount_sinceLastUpdate );
	UpdateFanfareFade( i_elapsedSecondCount_sinceLastUpdate );
	// The referee's whistle (only if the arrow is still waiting on the bow: a quick shot doesn't wait for it)
	if ( m_readyWhistleSecondCount >= 0.0f )
	{
		m_readyWhistleSecondCount -= i_elapsedSecondCount_sinceLastUpdate;
		if ( m_readyWhistleSecondCount < 0.0f )
		{
			if ( m_gameState == eGameState::Aiming )
			{
				PlayGameSound( m_assets.whistleSound, OneShot::Settings::Volume::whistle );
			}
		}
	}

	switch ( m_gameState )
	{
	case eGameState::Flying:
		CheckForShotResult();
		break;
	case eGameState::ShowingResult:
		UpdateResult( i_elapsedSecondCount_sinceLastUpdate );
		break;
	default:
		break;
	}

	// Both cameras follow the arrow (while it is on the bow that means the third-person camera is behind the archer)
	m_cameraRig.Track( m_arrow.GetPosition(), m_arrow.GetPosition() );
	m_cameraRig.Update( i_elapsedSecondCount_sinceLastUpdate );

	// The opening shot ends by itself
	// (this is checked after the cameras update so that the game is aiming in the same update that the opening shot ends)
	if ( ( m_gameState == eGameState::Intro ) && !m_cameraRig.IsIntroPlaying() )
	{
		StartAiming();
	}
}

void eae6320::cOneShotGame::SubmitDataToBeRendered( const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	const auto elapsed = i_elapsedSecondCount_sinceLastSimulationUpdate;
	m_cameraRig.SubmitToBeRendered( elapsed, m_arrow.GetVelocity() );

	Graphics::SubmitBackgroundColorForANewFrame( s_horizonColor[0], s_horizonColor[1], s_horizonColor[2] );
	// The sky dome is always centered on the camera so that the camera can never get close to its edge
	// (which makes it look infinitely far away)
	{
		Math::sVector cameraPosition;
		Math::cQuaternion cameraOrientation;
		float verticalFieldOfView;
		m_cameraRig.CalculateView( elapsed, m_arrow.GetVelocity(), cameraPosition, cameraOrientation, verticalFieldOfView );
		m_skyDome.SubmitToBeRendered( Math::cMatrix_transformation( Math::cQuaternion(), cameraPosition ) );
	}

	for ( auto& rangePart : m_range )
	{
		rangePart.SubmitToBeRendered( elapsed );
	}
	m_crowd.SubmitToBeRendered( elapsed );
	m_target.SubmitToBeRendered( elapsed );
	m_obstacleCourse.SubmitToBeRendered( elapsed );
	m_wind.SubmitToBeRendered( elapsed );
	m_quiver.SubmitToBeRendered( GetQuiverArrowCount() );
	m_scoreboard.SubmitToBeRendered( m_round, i_elapsedSecondCount_systemTime );
	m_timingLight.SubmitToBeRendered( m_gameState == eGameState::Aiming );
	if ( m_isShotArrowVisible )
	{
		m_arrow.SubmitToBeRendered( elapsed );
	}
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::cOneShotGame::Initialize()
{
	auto result = Results::Success;

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

	// The range never moves, and its meshes are already in world space
	for ( size_t i = 0; i < m_range.size(); ++i )
	{
		m_range[i].SetMesh( m_assets.rangeMeshes[i] );
		m_range[i].SetEffect( m_assets.litEffect );
		m_range[i].SetTexture( OneShot::sGameAssets::isRangeMeshGrass[i] ? m_assets.grassTexture : m_assets.whiteTexture );
	}
	// The sky isn't lit: it is the light source rather than something being lit
	m_skyDome.SetMesh( m_assets.skyDomeMesh );
	m_skyDome.SetEffect( m_assets.unlitEffect );
	m_skyDome.SetTexture( m_assets.whiteTexture );

	m_arrow.Initialize( m_assets );
	m_target.Initialize( m_assets );
	m_obstacleCourse.Initialize( m_assets );
	m_wind.Initialize( m_assets );
	m_scoreboard.Initialize( m_assets );
	m_quiver.Initialize( m_assets );
	m_crowd.Initialize( m_assets );
	m_timingLight.Initialize( m_assets );
	m_cameraRig.Track( m_arrow.GetPosition(), m_arrow.GetPosition() );
	m_round.Start();

	// The crowd murmurs the whole time, and the opening shot has a fanfare
	m_ambience = PlayGameSound( m_assets.ambienceSound, OneShot::Settings::Volume::ambience, false, true );
	PlayIntro();

	Logging::OutputMessage( "One Shot controls: Space skips the opening shot / launches / takes the next shot (or starts a new round),"
		" WASD steer, Shift boosts, F switches camera, O slow motion, P pause, R replays the opening shot" );

	return result;
}

eae6320::cResult eae6320::cOneShotGame::CleanUp()
{
	// Everything must be released here rather than in destructors
	// because this game object is destroyed after the Graphics system has shut down
	// (the sounds that are still playing are stopped and released by the audio system when it cleans up, which happens after this)
	for ( auto& rangePart : m_range )
	{
		rangePart.ReleaseAssets();
	}
	m_skyDome.ReleaseAssets();
	m_arrow.ReleaseAssets();
	m_target.ReleaseAssets();
	m_obstacleCourse.ReleaseAssets();
	m_wind.ReleaseAssets();
	m_scoreboard.ReleaseAssets();
	m_quiver.ReleaseAssets();
	m_crowd.ReleaseAssets();
	m_timingLight.ReleaseAssets();
	m_assets.Release();

	return Results::Success;
}

// Implementation
//===============

void eae6320::cOneShotGame::PlayIntro()
{
	m_cameraRig.PlayIntro();
	m_gameState = eGameState::Intro;
	m_readyWhistleSecondCount = -1.0f;
	// (A fanfare that is still playing or fading out from before is replaced)
	Audio::Stop( m_fanfare );
	m_fanfareVolume = OneShot::Settings::Volume::introFanfare;
	m_isFanfareFadingOut = false;
	m_fanfare = PlayGameSound( m_assets.introFanfareSound, m_fanfareVolume );
}

void eae6320::cOneShotGame::StartAiming()
{
	m_gameState = eGameState::Aiming;
	m_readyWhistleSecondCount = OneShot::Settings::readyWhistleDelay;
}

void eae6320::cOneShotGame::LaunchArrow()
{
	m_round.LaunchArrow();
	m_arrow.Launch();
	m_gameState = eGameState::Flying;
	m_cameraRig.SwitchTo( OneShot::cShotCameraRig::eView::FirstPerson, OneShot::Settings::cameraTransitionDuration );
	PlayGameSound( m_assets.bowReleaseSound, OneShot::Settings::Volume::bowRelease, true );
}

void eae6320::cOneShotGame::TakeAnotherShot()
{
	// After the last arrow the next shot starts a new round:
	// the arrows are pulled out of the target and the quiver is full again
	if ( m_round.IsOver() )
	{
		m_round.Start();
		m_target.RemoveStuckArrows();
		Logging::OutputMessage( "A new round has started" );
	}
	m_scoreboard.SetIsNewestScoreFlashing( false );
	m_crowd.Settle();
	m_isShotArrowVisible = true;

	m_arrow.Reset();
	m_obstacleCourse.Reset();
	StartAiming();
	// The arrow jumps back onto the bow, so the chase camera would have to travel the whole range to catch up
	// (flying through the obstacles on the way). Instead the cameras are moved straight to their new places,
	// and the view blends there from where it is now.
	m_cameraRig.Track( m_arrow.GetPosition(), m_arrow.GetPosition() );
	m_cameraRig.SettleTrackingCameras();
	m_cameraRig.SwitchTo( OneShot::cShotCameraRig::eView::ThirdPerson, OneShot::Settings::introSkipTransitionDuration, true );
}

void eae6320::cOneShotGame::CheckForShotResult()
{
	// The obstacles and the target are both tested, and whichever the arrow touched first during this update counts
	float timeOfImpact_obstacle = 1.0f;
	const auto isObstacleHit = m_obstacleCourse.IsHitBy( m_arrow, timeOfImpact_obstacle );
	float timeOfImpact_target = 1.0f;
	unsigned int score = 0;
	const auto isTargetHit = m_target.IsHitBy( m_arrow, timeOfImpact_target, score );

	if ( isObstacleHit && ( !isTargetHit || ( timeOfImpact_obstacle <= timeOfImpact_target ) ) )
	{
		// The arrow stops where it touched the obstacle (rather than wherever the update ended, which could be inside it)
		// and bounces back off it
		m_arrow.StopPartwayThroughLastUpdate( timeOfImpact_obstacle );
		m_arrow.Deflect();
		m_score = 0;
		EndShot( eShotResult::HitObstacle );
	}
	else if ( isTargetHit )
	{
		// The arrow sticks in the target where its tip crossed the face
		// (the target draws it from now on, pushed a little way into the straw)
		m_arrow.StopPartwayThroughLastUpdate( timeOfImpact_target );
		m_target.StickArrow( m_arrow.GetPosition() + Math::sVector( 0.0f, 0.0f, -OneShot::Settings::arrowEmbedDepth ) );
		m_isShotArrowVisible = false;
		m_score = score;
		EndShot( ( score > 0 ) ? eShotResult::HitTarget : eShotResult::HitBoss );
	}
	else if ( m_arrow.GetTipPosition().z < ( OneShot::Settings::targetPosition.z - OneShot::Settings::missDistancePastTarget ) )
	{
		// The arrow flew past the target into the net behind it
		m_arrow.Stop();
		m_isShotArrowVisible = false;
		m_score = 0;
		EndShot( eShotResult::Missed );
	}
}

void eae6320::cOneShotGame::EndShot( const eShotResult i_result )
{
	m_shotResult = i_result;
	m_gameState = eGameState::ShowingResult;
	m_resultSecondCount = 0.0f;
	// The result is shown at normal speed
	m_isSlowMotionOn = false;
	UpdateSimulationRate();
	// Pull back to the chase camera to show where the arrow ended up
	// (the result shot cuts to the crowd a moment later; see UpdateResult())
	m_cameraRig.SwitchTo( OneShot::cShotCameraRig::eView::ThirdPerson, OneShot::Settings::cameraTransitionDuration );

	// The score goes up on the big screen straight away, flashing until the next shot
	m_round.RecordScore( m_score );
	m_scoreboard.SetIsNewestScoreFlashing( true );

	// The sound of the arrow hitting something
	switch ( i_result )
	{
	case eShotResult::HitTarget:
	case eShotResult::HitBoss:
		PlayGameSound( m_assets.targetHitSound, OneShot::Settings::Volume::targetHit, true );
		if ( m_score == 10 )
		{
			PlayGameSound( m_assets.bullseyeSound, OneShot::Settings::Volume::bullseye );
		}
		break;
	case eShotResult::HitObstacle:
		PlayGameSound( m_assets.obstacleHitSound, OneShot::Settings::Volume::obstacleHit, true );
		break;
	default:
		// (A miss lands in the net far away, which wouldn't be heard)
		break;
	}

	switch ( i_result )
	{
	case eShotResult::HitTarget: Logging::OutputMessage( "The arrow hit the target and scored %u", m_score ); break;
	case eShotResult::HitBoss: Logging::OutputMessage( "The arrow hit the target's boss outside of the scoring rings" ); break;
	case eShotResult::HitObstacle: Logging::OutputMessage( "The arrow hit an obstacle" ); break;
	case eShotResult::Missed: Logging::OutputMessage( "The arrow missed the target" ); break;
	}
	if ( m_round.IsOver() )
	{
		Logging::OutputMessage( "The round is over with a total of %u (the best so far is %u); press Space to start a new round",
			m_round.GetTotal(), m_round.GetBestTotal() );
	}
}

void eae6320::cOneShotGame::UpdateResult( const float i_elapsedSecondCount_sinceLastUpdate )
{
	// Each event happens once, in the update when the time since the arrow landed passes the event's time
	const auto secondCount_before = m_resultSecondCount;
	m_resultSecondCount += i_elapsedSecondCount_sinceLastUpdate;
	const auto HasJustPassed = [secondCount_before, this]( const float i_time )
	{
		return ( secondCount_before < i_time ) && ( m_resultSecondCount >= i_time );
	};

	// The crowd reacts: they cheer a score (the better the score, the louder and more excited they are)
	// and groan at anything else
	if ( HasJustPassed( OneShot::Settings::crowdReactionDelay ) )
	{
		if ( m_shotResult == eShotResult::HitTarget )
		{
			const auto excitement = static_cast<float>( m_score ) / 10.0f;
			const auto volume = OneShot::Settings::Volume::cheerMinimum
				+ ( ( OneShot::Settings::Volume::cheerMaximum - OneShot::Settings::Volume::cheerMinimum ) * ( ( static_cast<float>( m_score ) - 1.0f ) / 9.0f ) );
			PlayGameSound( m_assets.cheerSound, volume );
			m_crowd.Cheer( 0.4f + ( 0.6f * excitement ) );
		}
		else
		{
			const auto volume = ( m_shotResult == eShotResult::HitObstacle )
				? OneShot::Settings::Volume::groan_obstacle : OneShot::Settings::Volume::groan_miss;
			PlayGameSound( m_assets.groanSound, volume );
		}
	}
	// Cut to the crowd, then on to the big screen
	if ( HasJustPassed( OneShot::Settings::resultShotDelay ) )
	{
		m_cameraRig.PlayResultShot();
	}
	// A fanfare for finishing the round, as the result shot reaches the big screen
	if ( HasJustPassed( OneShot::Settings::roundCompleteFanfareDelay ) && m_round.IsOver() )
	{
		PlayGameSound( m_assets.roundCompleteSound, OneShot::Settings::Volume::roundComplete );
	}
	// Back to the archer for the next shot (or the next round) once the big screen has been shown for a while
	{
		const auto timeWhenTheScreenIsReached = OneShot::Settings::resultShotDelay + OneShot::Settings::resultKeyframes.back().time;
		const auto holdDuration = m_round.IsOver()
			? OneShot::Settings::scoreboardHoldDuration_roundOver : OneShot::Settings::scoreboardHoldDuration;
		if ( m_resultSecondCount >= ( timeWhenTheScreenIsReached + holdDuration ) )
		{
			TakeAnotherShot();
		}
	}
}

unsigned int eae6320::cOneShotGame::GetQuiverArrowCount() const
{
	// Before the arrow is launched one of the arrows that are left is on the bow, not in the quiver
	const auto arrowsLeft = m_round.GetArrowsLeft();
	const auto isAnArrowOnTheBow = ( m_gameState == eGameState::Intro ) || ( m_gameState == eGameState::Aiming );
	return ( isAnArrowOnTheBow && ( arrowsLeft > 0 ) ) ? ( arrowsLeft - 1 ) : arrowsLeft;
}

eae6320::Audio::sPlayingSound eae6320::cOneShotGame::PlayGameSound( Audio::cSound* const i_sound, const float i_volume,
	const bool i_shouldVaryPitch, const bool i_shouldLoop )
{
	if ( !i_sound )
	{
		return {};
	}
	Audio::sPlaybackSettings settings;
	settings.volume = i_volume;
	settings.shouldLoop = i_shouldLoop;
	if ( i_shouldVaryPitch )
	{
		std::uniform_real_distribution<float> variation( -OneShot::Settings::pitchVariation, OneShot::Settings::pitchVariation );
		settings.pitch = 1.0f + variation( m_randomNumberGenerator );
	}
	return i_sound->Play( settings );
}

void eae6320::cOneShotGame::StartFanfareFadeOut()
{
	if ( Audio::IsPlaying( m_fanfare ) )
	{
		m_isFanfareFadingOut = true;
	}
}

void eae6320::cOneShotGame::UpdateFanfareFade( const float i_elapsedSecondCount_sinceLastUpdate )
{
	if ( !m_isFanfareFadingOut )
	{
		return;
	}
	// The volume goes down a step every simulation update
	// (XAudio2 smooths each change over a few milliseconds, so the steps aren't heard as steps)
	m_fanfareVolume -= ( OneShot::Settings::Volume::introFanfare / OneShot::Settings::fanfareFadeOutDuration ) * i_elapsedSecondCount_sinceLastUpdate;
	if ( m_fanfareVolume > 0.0f )
	{
		Audio::SetVolume( m_fanfare, m_fanfareVolume );
	}
	else
	{
		// It is silent by now, so stopping it can't make a click
		Audio::Stop( m_fanfare );
		m_fanfareVolume = 0.0f;
		m_isFanfareFadingOut = false;
	}
}

void eae6320::cOneShotGame::UpdateSimulationRate()
{
	// The application's simulation rate scales how much simulation time passes per real second:
	// 0 stops the simulation completely, and anything between 0 and 1 is slow motion.
	// Everything that uses simulation time (movement, the cameras, even the extrapolation used for rendering)
	// slows down together, which wouldn't be true if the game scaled the elapsed time itself.
	SetSimulationRate( m_isPaused ? 0.0f : ( m_isSlowMotionOn ? OneShot::Settings::slowMotionRate : 1.0f ) );
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
