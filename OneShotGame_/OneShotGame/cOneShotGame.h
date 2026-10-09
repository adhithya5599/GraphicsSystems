/*
	One Shot: launch an arrow, steer it past the moving obstacles, and hit the target.
	You only get one shot.

	This class is the glue between the engine and the game:
	it owns the game's pieces and decides what happens when (the game's states),
	while each piece handles its own details:
		* sGameAssets loads every mesh, effect, and texture once
		* cArrow flies and steers the arrow
		* cObstacleCourse moves the obstacles and checks whether the arrow touched one
		* cShotCameraRig follows the action and blends between the two cameras
		* cKeyPressTracker turns held keys into single presses
		* OneShotLock remembers that the shot has been taken
	Every number that tunes the game is in GameplaySettings.h.
*/

#ifndef EAE6320_CONESHOTGAME_H
#define EAE6320_CONESHOTGAME_H

// Includes
//=========

#include "cArrow.h"
#include "cKeyPressTracker.h"
#include "cObstacleCourse.h"
#include "cShotCameraRig.h"
#include "sGameAssets.h"

#include <Engine/Application/iApplication.h>
#include <Engine/GameObject/cGameObject.h>
#include <Engine/Results/Results.h>

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
		// The following three icons are provided:
		//	* IDI_EAEGAMEPAD
		//	* IDI_EAEALIEN
		//	* IDI_VSDEFAULT_LARGE / IDI_VSDEFAULT_SMALL
		// If you want to try creating your own a convenient website that will help is: http://icoconvert.com/
		const WORD* GetLargeIconId() const final { static constexpr WORD iconId_large = IDI_EAEGAMEPAD; return &iconId_large; }
		const WORD* GetSmallIconId() const final { static constexpr WORD iconId_small = IDI_EAEGAMEPAD; return &iconId_small; }
#endif

		// Run
		//----

		// Every frame: keys that toggle things or start things (pause, slow motion, launch, switch camera)
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
			// The game won't start because this computer has already taken its shot
			AlreadyPlayed,
			// Waiting for the player to launch the arrow
			Aiming,
			// The arrow is in the air and the player is steering it
			Flying,
			// The arrow hit something (or missed); the result is shown for a moment and then the game closes
			ShowingResult,
		};
		enum class eShotResult
		{
			HitTarget,
			HitObstacle,
			Missed,
		};

		eGameState m_gameState = eGameState::Aiming;
		eShotResult m_shotResult = eShotResult::Missed;
		// How long the result has been shown for
		float m_resultSecondCount = 0.0f;

		OneShot::sGameAssets m_assets;
		OneShot::cArrow m_arrow;
		OneShot::cObstacleCourse m_obstacleCourse;
		OneShot::cShotCameraRig m_cameraRig;
		GameObject::cGameObject m_bow;
		GameObject::cGameObject m_ground;
		GameObject::cGameObject m_target;
		GameObject::cGameObject m_skybox;

		// Input
		//------

		OneShot::cKeyPressTracker m_keyPresses;
		// Key presses are noticed every frame,
		// but anything that changes the simulation waits for the next simulation update
		// (see UpdateSimulationBasedOnInput() in iApplication.cpp for why)
		bool m_isLaunchRequested = false;
		bool m_isViewSwitchRequested = false;
		bool m_isPaused = false;
		bool m_isSlowMotionOn = false;
		// Exiting doesn't stop the application loop immediately,
		// so this makes sure that the game only tries to exit (and shows its closing message) once
		bool m_isExiting = false;

		// Implementation
		//===============

		void LaunchArrow();
		void CheckForShotResult();
		void EndShot( const eShotResult i_result );
		// Pausing and slow motion both just change how fast simulation time passes
		void UpdateSimulationRate();
		void ExitGame();
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
