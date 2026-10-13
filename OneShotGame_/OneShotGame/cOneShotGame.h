/*
	One Shot: an Olympic archery range with obstacles.
	Watch the range, launch the arrow, steer it past the obstacles and the wind, and hit the target.
	A round is six arrows; the big screen keeps score, and the crowd cheers a hit and groans at a miss.

	This class is the glue between the engine and the game:
	it owns the game's pieces and decides what happens when (the game's states),
	while each piece handles its own details:
		* sGameAssets loads every mesh, effect, texture, and sound once
		* cArrow flies and steers the arrow
		* cTarget places the targets and scores a hit
		* cObstacleCourse moves the obstacles and checks whether the arrow touched one
		* cWind blows the arrow sideways and turns the flags
		* cShotCameraRig plays the opening shot, follows the action, blends between cameras, and plays the result shot
		* cRound keeps the round's score, which cScoreboard shows on the big screen and cQuiver shows as arrows in the quiver
		* cCrowd makes the crowd jump when it cheers
		* cTimingLight shows when the player can shoot (green with "SPACE", or red)
		* cKeyPressTracker turns held keys into single presses
	Every number that tunes the game is in GameplaySettings.h.
*/

#ifndef EAE6320_CONESHOTGAME_H
#define EAE6320_CONESHOTGAME_H

// Includes
//=========

#include "cArrow.h"
#include "cCrowd.h"
#include "cKeyPressTracker.h"
#include "cObstacleCourse.h"
#include "cQuiver.h"
#include "cRound.h"
#include "cScoreboard.h"
#include "cShotCameraRig.h"
#include "cTarget.h"
#include "cTimingLight.h"
#include "cWind.h"
#include "sGameAssets.h"

#include <Engine/Application/iApplication.h>
#include <Engine/Audio/Audio.h>
#include <Engine/GameObject/cGameObject.h>
#include <Engine/Results/Results.h>

#include <array>
#include <random>

#if defined( EAE6320_PLATFORM_WINDOWS )
	#include "Resource Files/Resource.h"
#endif

// Class Declaration
//==================

namespace eae6320
{
	class cOneShotGame final : public Application::iApplication
	{
		// Inherited Implementation
		//=========================

	private:

		// Configuration
		//--------------

#if defined( EAE6320_PLATFORM_WINDOWS )
		// The main window's name will be displayed as its caption (the text that is displayed in the title bar).
		// You can make it anything that you want, but please keep the platform name and debug configuration at the end
		// so that it's easy to tell at a glance what kind of build is running.
		const char* GetMainWindowName() const final
		{
			return "One Shot Game"
				" -- "
#if defined( EAE6320_PLATFORM_D3D )
				"Direct3D"
#elif defined( EAE6320_PLATFORM_GL )
				"OpenGL"
#endif
#ifdef _DEBUG
				" -- Debug"
#endif
			;
		}
		// Window classes are almost always identified by name;
		// there is a unique "ATOM" associated with them,
		// but in practice Windows expects to use the class name as an identifier.
		// If you don't change the name below
		// your program could conceivably have problems if it were run at the same time on the same computer
		// as one of your classmate's.
		// You don't need to worry about this for our class,
		// but if you ever ship a real project using this code as a base you should set this to something unique
		// (a generated GUID would be fine since this string is never seen)
		const char* GetMainWindowClassName() const final { return "Adhithya's EAE6320 One Shot Main Window Class"; }
		// The game's icon is a target with an arrow in it (see Resource Files/Resource.h).
		// One icon file has every size in it, so it is used for both the large and the small icon
		// and Windows picks the size it needs for each.
		const WORD* GetLargeIconId() const final { static constexpr WORD iconId_large = IDI_TARGET; return &iconId_large; }
		const WORD* GetSmallIconId() const final { static constexpr WORD iconId_small = IDI_TARGET; return &iconId_small; }
#endif
		// The game fills the screen: a small square window cuts off the sides of the view
		// (the timing light at the edge of the aiming view needs a widescreen field of view).
		// Esc quits, and Alt+Tab still switches to other windows.
		bool ShouldMainWindowFillTheScreen() const final { return true; }

		// Run
		//----

		// Every frame: keys that toggle things or start things (pause, slow motion, launch, skip, switch camera)
		void UpdateBasedOnInput() final;
		// Every simulation update: steering, and acting on what was requested in UpdateBasedOnInput()
		void UpdateSimulationBasedOnInput() final;
		void UpdateSimulationBasedOnTime( const float i_elapsedSecondCount_sinceLastUpdate ) final;
		void SubmitDataToBeRendered( const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate ) final;

		// Initialize / Clean Up
		//----------------------

		cResult Initialize() final;
		cResult CleanUp() final;

		// Data
		//=====

	private:

		enum class eGameState
		{
			// The opening shot is sweeping over the range
			Intro,
			// Waiting for the player to launch the arrow
			Aiming,
			// The arrow is in the air and the player is steering it
			Flying,
			// The arrow hit something (or missed): the crowd reacts, the view cuts to them and then to the big screen,
			// and this lasts until the player takes the next shot (or starts a new round after the last arrow)
			ShowingResult,
		};
		enum class eShotResult
		{
			// The arrow hit the target's face (it scored 1 to 10)
			HitTarget,
			// The arrow hit the target's straw boss but outside of the scoring rings
			HitBoss,
			HitObstacle,
			Missed,
		};

		eGameState m_gameState = eGameState::Intro;
		eShotResult m_shotResult = eShotResult::Missed;
		// The points that the last arrow scored
		unsigned int m_score = 0;
		// How long the result has been shown for
		// (everything that happens after an arrow lands is timed from this; see UpdateResult())
		float m_resultSecondCount = 0.0f;
		// Whether the arrow that was shot is drawn
		// (it isn't once it's stuck in the target, which draws it instead, or once it has missed and gone into the net)
		bool m_isShotArrowVisible = true;

		OneShot::sGameAssets m_assets;
		// The range itself (the ground, lines, stands, surroundings, and the archer), which never moves
		std::array<GameObject::cGameObject, OneShot::sGameAssets::rangeMeshCount> m_range;
		GameObject::cGameObject m_skyDome;
		OneShot::cArrow m_arrow;
		OneShot::cTarget m_target;
		OneShot::cObstacleCourse m_obstacleCourse;
		OneShot::cWind m_wind;
		OneShot::cShotCameraRig m_cameraRig;
		OneShot::cRound m_round;
		OneShot::cScoreboard m_scoreboard;
		OneShot::cQuiver m_quiver;
		OneShot::cCrowd m_crowd;
		OneShot::cTimingLight m_timingLight;

		// Audio
		//------

		// The crowd's murmuring, which loops the whole time
		Audio::sPlayingSound m_ambience;
		// The opening shot's fanfare, which fades out if the opening shot is skipped
		Audio::sPlayingSound m_fanfare;
		float m_fanfareVolume = 0.0f;
		bool m_isFanfareFadingOut = false;
		// How long until the referee's whistle blows (negative when it isn't going to)
		float m_readyWhistleSecondCount = -1.0f;
		// For varying the pitch of sounds that play often
		std::minstd_rand m_randomNumberGenerator;

		// Input
		//------

		OneShot::cKeyPressTracker m_keyPresses;
		// Key presses are noticed every frame,
		// but anything that changes the simulation waits for the next simulation update
		// (see UpdateSimulationBasedOnInput() in iApplication.cpp for why)
		bool m_isSpaceRequested = false;
		// What Space should do depends on the state when it was pressed
		// (the state can change between the press and the next simulation update, e.g. when the opening shot ends,
		// and a press meant to skip the opening shot mustn't launch the arrow instead)
		eGameState m_gameState_whenSpaceWasPressed = eGameState::Intro;
		bool m_isViewSwitchRequested = false;
		bool m_isIntroReplayRequested = false;
		bool m_isPaused = false;
		bool m_isSlowMotionOn = false;

		// Implementation
		//===============

		void PlayIntro();
		// The arrow is on the bow and can be launched: the timing light turns green and the whistle blows a moment later
		void StartAiming();
		void LaunchArrow();
		void TakeAnotherShot();
		void CheckForShotResult();
		void EndShot( const eShotResult i_result );
		// Does whatever is due at this point after the arrow landed (the crowd reacting, the result shot, the end of the round)
		void UpdateResult( const float i_elapsedSecondCount_sinceLastUpdate );
		// How many arrows to draw in the quiver
		unsigned int GetQuiverArrowCount() const;

		// Plays a sound if it was loaded (a sound that couldn't be loaded is just silent).
		// (It isn't called PlaySound() because Windows has a macro with that name)
		// i_shouldVaryPitch plays it slightly higher or lower each time.
		Audio::sPlayingSound PlayGameSound( Audio::cSound* const i_sound, const float i_volume, const bool i_shouldVaryPitch = false,
			const bool i_shouldLoop = false );
		void StartFanfareFadeOut();
		void UpdateFanfareFade( const float i_elapsedSecondCount_sinceLastUpdate );
		// Pausing and slow motion both just change how fast simulation time passes
		void UpdateSimulationRate();
	};
}

// Result Definitions
//===================

namespace eae6320
{
	namespace Results
	{
		namespace Application
		{
			// You can add specific results for your game here:
			//	* The System should always be Application
			//	* The __LINE__ macro is used to make sure that every result has a unique ID.
			//		That means, however, that all results _must_ be defined in this single file
			//		or else you could have two different ones with equal IDs.
			//	* Note that you can define multiple Success codes.
			//		This can be used if the caller may want to know more about how a function succeeded.
			constexpr cResult ExampleResult( IsFailure, eSystem::Application, __LINE__, Severity::Default );
		}
	}
}

#endif	// EAE6320_CONESHOTGAME_H
