/*
	Every number that tunes how One Shot plays, in one place

	Coordinates are in the engine's world space: +x is right, +y is up, and the arrow flies down -z toward the target.

	The collider sizes come from the meshes' bounds (OneShotGame_/Content/Meshes),
	measured in each mesh's own local space.
	The Maya meshes were not modeled around their origins,
	so a collider's center is often offset from the object's position
	(e.g. the obstacle mesh sits 4 units in front of its origin).
	If a mesh is re-exported with a different pivot, its collider here must be updated to match.
*/

#ifndef EAE6320_ONESHOT_GAMEPLAYSETTINGS_H
#define EAE6320_ONESHOT_GAMEPLAYSETTINGS_H

// Includes
//=========

#include <Engine/Math/sVector.h>

#include <array>

// Settings
//=========

namespace eae6320
{
	namespace OneShot
	{
		namespace Settings
		{
			// The Bow and Arrow
			//==================

			// Where the bow stands (the arrow rests on it before it is launched)
			constexpr Math::sVector bowPosition( 0.0f, 0.0f, 0.0f );

			// How fast the arrow flies forward (units per second)
			constexpr float arrowSpeed_normal = 5.0f;
			// Holding Shift boosts the arrow (but not while time is slowed, so slow motion can't be "cheated")
			constexpr float arrowSpeed_boosted = 10.0f;
			// How fast WASD move the arrow up/down/left/right (units per second)
			constexpr float arrowSteeringSpeed = 2.5f;
			// The arrow can't be steered outside of this box (only x and y are used)
			constexpr Math::sVector arrowSteeringLimit_minimum( -8.0f, 0.0f, 0.0f );
			constexpr Math::sVector arrowSteeringLimit_maximum( 8.0f, 6.0f, 0.0f );

			// The arrow's colliders, in the arrow mesh's local space
			// (the arrow mesh points down -z with its tip at z = -0.64).
			// The tip is a small sphere that is swept along the arrow's path every update
			// so that a fast arrow can't pass through something thin between two updates,
			// and the shaft is a box so that steering sideways into something also counts as a hit.
			constexpr Math::sVector arrowTip_local( 0.125f, 0.285f, -0.64f );
			constexpr float arrowTipRadius = 0.1f;
			constexpr Math::sVector arrowShaftCenter_local( 0.125f, 0.285f, 0.875f );
			constexpr Math::sVector arrowShaftHalfExtents( 0.075f, 0.075f, 1.515f );

			// The Target
			//===========

			constexpr Math::sVector targetPosition( -2.5f, 0.0f, -35.0f );
			// The target's collider, in the target mesh's local space
			constexpr Math::sVector targetColliderCenter_local( 0.0f, 0.16f, -0.615f );
			constexpr Math::sVector targetColliderHalfExtents( 1.34f, 2.83f, 0.215f );
			// Once the arrow's tip is this far past the target the shot has missed
			constexpr float missDistancePastTarget = 3.0f;

			// The Obstacles
			//==============

			// Each obstacle moves back and forth along one axis around its starting position
			enum class eObstacleMotionAxis
			{
				X,
				Y,
				Z,
			};

			struct sObstacleLayout
			{
				Math::sVector position;
				eObstacleMotionAxis motionAxis;
				// How far it moves away from its starting position in each direction
				float amplitude;
				// How quickly it moves back and forth (in radians per second; 2 * pi radians is one full cycle)
				float angularFrequency;
			};

			// (The obstacle mesh is 4 units in front of its origin,
			// so an obstacle at z = -12 is actually seen between z = -15 and z = -17)
			constexpr std::array<sObstacleLayout, 7> obstacleLayouts =
			{ {
				{ Math::sVector( -3.0f, 1.5f, -12.0f ), eObstacleMotionAxis::Z, 1.5f, 0.5f },
				{ Math::sVector( -1.5f, 2.0f, -10.0f ), eObstacleMotionAxis::X, 1.8f, 0.6f },
				{ Math::sVector( 0.0f, 1.0f, -17.0f ), eObstacleMotionAxis::Y, 2.1f, 0.5f },
				{ Math::sVector( 1.5f, 2.5f, -25.0f ), eObstacleMotionAxis::Z, 1.5f, 0.6f },
				{ Math::sVector( 3.0f, 1.5f, -27.0f ), eObstacleMotionAxis::X, 1.8f, 0.5f },
				// The original layout only had five depths for seven obstacles
				// (the last two read past the end of the array, so where they appeared was undefined);
				// these two depths fill the gaps in the middle of the course
				{ Math::sVector( -3.0f, 1.5f, -20.0f ), eObstacleMotionAxis::Y, 2.1f, 0.6f },
				{ Math::sVector( -1.5f, 2.0f, -22.5f ), eObstacleMotionAxis::Z, 1.5f, 0.5f },
			} };

			// The obstacle's collider, in the obstacle mesh's local space
			constexpr Math::sVector obstacleColliderCenter_local( 0.0f, 0.0f, -4.0f );
			constexpr Math::sVector obstacleColliderHalfExtents( 1.0f, 1.4f, 1.0f );

			// The Cameras
			//============

			// The third-person camera follows this point relative to whatever it is tracking
			// (the bow while aiming, and then the arrow)
			constexpr Math::sVector thirdPersonTargetOffset( 0.0f, 2.0f, 12.0f );
			// The first-person camera sits just above and behind the arrow's tip
			constexpr Math::sVector firstPersonTargetOffset( 0.0f, 0.2f, 0.75f );
			// How long it takes to blend from one camera to the other
			constexpr float cameraTransitionDuration = 0.6f;

			// Time
			//=====

			// How fast time passes while slow motion is on (1 is normal speed)
			constexpr float slowMotionRate = 0.1f;
			// How long the result is shown before the game closes (in seconds of simulation time)
			constexpr float resultDisplayDuration = 3.0f;
		}
	}
}

#endif	// EAE6320_ONESHOT_GAMEPLAYSETTINGS_H
