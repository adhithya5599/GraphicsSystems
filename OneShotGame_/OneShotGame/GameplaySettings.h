/*
	Every number that tunes how One Shot plays, in one place

	Units are meters and seconds. Coordinates are in the engine's world space:
	+x is right, +y is up, and the arrow flies down -z toward the target, which is 70 m away (an Olympic distance).

	The meshes come from OneShotGame_/Content/Blender/build_archery_range.py.
	Each mesh is modeled around its own origin and placed by the game using the positions below,
	so the positions here are the only place the layout is defined
	(the build script has a copy of them only so that the .blend file shows the same layout and bakes its shadows correctly;
	if a position changes here, change it there too and rebuild the meshes).
	The collider sizes are measured from those meshes, in each mesh's own local space.
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
			// The Round
			//==========

			// A round is one "end" of Olympic archery: six arrows, each scoring up to 10 points
			constexpr unsigned int arrowsPerRound = 6;

			// The Arrow
			//==========

			// Where the arrow rests on the bow, at full draw (this is the arrow's nock; its tip is 0.75 m in front)
			constexpr Math::sVector arrowRestPosition( 0.0f, 1.5f, 0.0f );

			// How fast the arrow flies down the range (a real arrow flies at about 70 m/s;
			// this is slowed down so that it can be steered: about 9 seconds to the target)
			constexpr float arrowSpeed_normal = 8.0f;
			// Holding Shift boosts the arrow (but not in slow motion, so slow motion can't be used to cheat)
			constexpr float arrowSpeed_boosted = 15.0f;
			// How fast WASD move the arrow up/down/left/right
			constexpr float arrowSteeringSpeed = 3.0f;
			// The arrow can't be steered outside of this box (only x and y are used)
			constexpr Math::sVector arrowSteeringLimit_minimum( -6.0f, 0.4f, 0.0f );
			constexpr Math::sVector arrowSteeringLimit_maximum( 6.0f, 5.5f, 0.0f );

			// The arrow's colliders, in the arrow mesh's local space (the nock is at the origin and the tip at z = -0.75).
			// The tip is a small sphere that is swept along the arrow's path every update
			// so that a fast arrow can't pass through something thin between two updates,
			// and the shaft is a box so that steering sideways into something also counts as a hit.
			constexpr Math::sVector arrowTip_local( 0.0f, 0.0f, -0.75f );
			constexpr float arrowTipRadius = 0.03f;
			constexpr Math::sVector arrowShaftCenter_local( 0.0f, 0.0f, -0.375f );
			constexpr Math::sVector arrowShaftHalfExtents( 0.02f, 0.02f, 0.375f );

			// The Target
			//===========

			// The point on the ground under the center of the face
			constexpr Math::sVector targetPosition( 0.0f, 0.0f, -70.0f );
			// The other lanes' targets are only scenery (they are outside of the area the arrow can be steered in)
			constexpr std::array<float, 4> neighborTargetXs = { -15.0f, -10.0f, 10.0f, 15.0f };
			// The face (122 cm across) is on the front of a 130 cm square boss that leans back by 12 degrees
			constexpr float targetFaceCenterHeight = 1.3f;
			constexpr float targetBossHalfSize = 0.65f;
			constexpr float targetFaceRadius = 0.61f;
			constexpr float targetTiltDegrees = 12.0f;
			// Once the arrow's tip is this far past the target the shot has missed
			constexpr float missDistancePastTarget = 3.0f;

			// The Obstacles
			//==============
			// (Each obstacle is a fixed frame plus a moving wooden part; see cObstacleCourse)

			// A plank hanging from a trolley that slides left and right along a rail between two posts
			namespace RailSlider
			{
				constexpr float z = -20.0f;
				constexpr float amplitude = 4.2f;		// How far it slides from the middle in each direction
				constexpr float periodInSeconds = 4.5f;	// How long one full slide there and back takes
				constexpr float phase = 0.0f;			// Where in its slide it starts (0 to 1)
				// The plank's collider (in the plank mesh's local space, which has its origin on the ground)
				constexpr Math::sVector plankCenter_local( 0.0f, 2.15f, 0.0f );
				constexpr Math::sVector plankHalfExtents( 1.0f, 0.75f, 0.03f );
			}

			// A sandbag bunker with two boards hidden behind it that spring up when the arrow gets close
			namespace Bunker
			{
				constexpr float z = -33.0f;
				// The boards are just behind the sandbags
				constexpr float boardZ = z - 0.6f;
				constexpr std::array<float, 2> boardXs = { -1.6f, 1.6f };
				// Each board starts rising when the arrow's tip is this far in front of it
				constexpr std::array<float, 2> boardTriggerDistances = { 16.0f, 11.0f };
				// The board's bottom edge while hidden and once up (only the red band on top shows above the sandbags while hidden)
				constexpr float boardHiddenHeight = -0.8f;
				constexpr float boardRaisedHeight = 0.8f;
				constexpr float boardRiseDurationInSeconds = 0.5f;
				// The board's collider (in the board mesh's local space, which has its origin at the middle of the bottom edge)
				constexpr Math::sVector boardCenter_local( 0.0f, 0.9f, 0.0f );
				constexpr Math::sVector boardHalfExtents( 0.7f, 0.9f, 0.04f );
				// The sandbag wall's collider
				constexpr Math::sVector wallCenter_local( 0.0f, 0.45f, 0.0f );
				constexpr Math::sVector wallHalfExtents( 4.0f, 0.45f, 0.4f );
			}

			// A board hanging on two ropes from a gantry, raised and lowered by pulleys
			namespace Pulley
			{
				constexpr float z = -48.0f;
				// The height of the board's top edge: it moves between (middle - amplitude) and (middle + amplitude)
				constexpr float middleHeight = 3.6f;
				constexpr float amplitude = 2.0f;
				constexpr float periodInSeconds = 6.0f;
				constexpr float phase = 0.25f;
				// Where the ropes come down from the pulleys (in the frame's local space)
				constexpr std::array<float, 2> ropeXs = { -0.9f, 0.9f };
				constexpr float ropeTopHeight = 6.25f;
				// The board's collider (in the board mesh's local space, which has its origin at the middle of the top edge)
				constexpr Math::sVector boardCenter_local( 0.0f, -0.7f, 0.0f );
				constexpr Math::sVector boardHalfExtents( 1.2f, 0.7f, 0.03f );
				// The gantry's two posts are inside the area the arrow can fly through
				constexpr std::array<float, 2> postXs = { -3.2f, 3.2f };
				constexpr Math::sVector postHalfExtents( 0.125f, 3.4f, 0.125f );
			}

			// A plank on a sled that slides back and forth along tracks on the ground
			namespace Sled
			{
				constexpr float z = -60.0f;
				constexpr float amplitude = 3.5f;
				constexpr float periodInSeconds = 3.8f;
				constexpr float phase = 0.6f;
				constexpr Math::sVector plankCenter_local( 0.0f, 0.8f, 0.0f );
				constexpr Math::sVector plankHalfExtents( 0.9f, 0.65f, 0.03f );
			}

			// The Wind
			//=========

			// The flags show the wind (they are outside of the area the arrow can be steered in)
			constexpr std::array<Math::sVector, 3> flagPolePositions =
			{
				Math::sVector( -7.0f, 0.0f, -12.0f ),
				Math::sVector( 7.0f, 0.0f, -40.0f ),
				Math::sVector( -7.0f, 0.0f, -66.0f ),
			};
			// The crosswind's speed (along x) is the sum of two slow waves, so it gusts and changes direction
			// in a pattern that can be read from the flags and planned for
			constexpr float windSlowAmplitude = 1.6f;
			constexpr float windSlowPeriodInSeconds = 20.0f;
			constexpr float windFastAmplitude = 0.7f;
			constexpr float windFastPeriodInSeconds = 6.5f;
			// How much of the wind's speed is added to the arrow's sideways speed
			constexpr float windDriftFactor = 0.6f;
			// The wind speed at which a flag is fully stretched out
			constexpr float windSpeedForFullFlag = 2.0f;

			// The Cameras
			//============

			// The third-person camera follows this point relative to the arrow
			// (from thirdPersonFollowCamera.lua it sits behind and above that point, looking at it):
			// a point some way down the range keeps the view looking down the range rather than down at the arrow
			constexpr Math::sVector thirdPersonTargetOffset( 0.0f, 0.5f, -14.0f );
			// The first-person camera sits just above and behind the arrow's nock
			constexpr Math::sVector firstPersonTargetOffset( 0.0f, 0.1f, 0.55f );
			// How long it takes to blend from one camera to another
			constexpr float cameraTransitionDuration = 0.6f;
			// Skipping the opening shot blends to the aiming view a little more slowly so it doesn't feel like a cut
			constexpr float introSkipTransitionDuration = 0.9f;

			// The scripted camera moves are made of keyframes: where the camera is at a moment and what it is looking at
			// (see Engine/CameraControls/cCameraPath.h for how the camera moves between them)
			struct sCameraKeyframe
			{
				float time;
				Math::sVector position;
				Math::sVector lookAtPosition;
			};

			// The opening shot: a sweep over the whole range, ending exactly where the aiming camera is.
			// It starts high above the grandstand, drops to the target, and then pulls back up the range past each obstacle
			// (always looking down the range: if the camera ever passed over the point it looks at, it would have to spin around).
			constexpr std::array<sCameraKeyframe, 6> introKeyframes =
			{ {
				// High above the grandstand: the whole range, the crowd, and the hills
				{ 0.0f, Math::sVector( 40.0f, 26.0f, -20.0f ), Math::sVector( 0.0f, 0.0f, -40.0f ) },
				// Down to the target face, from just in front of it
				{ 2.8f, Math::sVector( 6.0f, 3.5f, -58.0f ), Math::sVector( 0.0f, 1.3f, -70.0f ) },
				// Pulling back past the pulley gantry (outside its posts and above the flags)
				{ 5.0f, Math::sVector( 10.5f, 5.5f, -41.0f ), Math::sVector( 0.0f, 2.5f, -48.0f ) },
				// The front of the bunker (its boards are hidden until the arrow comes)
				{ 7.2f, Math::sVector( 8.0f, 4.0f, -24.0f ), Math::sVector( 0.0f, 1.2f, -33.0f ) },
				// The rail slider (high enough to clear its rail and posts)
				{ 9.2f, Math::sVector( 2.0f, 4.8f, -8.0f ), Math::sVector( 0.0f, 2.4f, -20.0f ) },
				// Settling behind the archer, exactly where the aiming camera is (see thirdPersonFollowCamera.lua)
				{ 11.5f, Math::sVector( 1.2f, 2.9f, 4.0f ), Math::sVector( 0.0f, 2.0f, -14.0f ) },
			} };

			// The result shot: after each arrow lands the view cuts to the crowd (like a television broadcast),
			// moves along the front of the grandstand while they react, and then turns to the big screen to show the new score
			constexpr std::array<sCameraKeyframe, 3> resultKeyframes =
			{ {
				// The crowd at the near end of the grandstand
				{ 0.0f, Math::sVector( 27.0f, 3.6f, -20.0f ), Math::sVector( 40.0f, 4.0f, -28.0f ) },
				// Moving along the stands
				{ 2.2f, Math::sVector( 27.0f, 3.6f, -38.0f ), Math::sVector( 40.0f, 4.0f, -46.0f ) },
				// Turning to the big screen, square on to it so that the numbers are easy to read
				{ 4.2f, Math::sVector( 16.0f, 6.8f, -52.0f ), Math::sVector( 21.0f, 7.0f, -72.6f ) },
			} };

			// Time
			//=====

			// How fast time passes while slow motion is on (1 is normal speed)
			constexpr float slowMotionRate = 0.1f;

			// The Result
			//===========
			// What happens after an arrow lands, in seconds after it landed

			// The crowd reacts (cheers or groans) a moment after the arrow lands, the way real people do
			constexpr float crowdReactionDelay = 0.4f;
			// The view stays on the arrow for a moment and then cuts to the crowd
			constexpr float resultShotDelay = 1.3f;
			// After the last arrow of a round, a short fanfare plays as the result shot reaches the big screen
			constexpr float roundCompleteFanfareDelay = 4.7f;
			// How long the result must be shown before Space takes the next shot
			// (long enough that a press meant for the flight doesn't skip the result, short enough that the crowd shot can be skipped)
			constexpr float resultMinimumDisplayDuration = 1.5f;
			// After the last arrow of a round the result must be shown until the big screen is in view,
			// so that the final total can't be skipped past (the next press starts a new round, which clears it)
			constexpr float roundOverMinimumDisplayDuration = 5.0f;
			// Once the result shot reaches the big screen it stays there this long and then goes back to the archer by itself
			// (Space still skips it); after the last arrow of a round it stays longer so the final total can be read,
			// and then a new round starts
			constexpr float scoreboardHoldDuration = 2.5f;
			constexpr float scoreboardHoldDuration_roundOver = 4.5f;

			// The Crowd
			//==========

			// When the crowd cheers, each group of people hops up and down, a little out of step with the other groups
			constexpr float crowdHopHeight = 0.22f;
			constexpr float crowdHopsPerSecond = 2.4f;
			constexpr float crowdCheerDuration = 3.5f;

			// The Big Screen
			//===============
			// (The screen's mesh is built by OneShotGame_/Content/Blender/build_archery_range.py with the layout in scoreboard_layout.py;
			// the numbers here must match the ones there)

			namespace Scoreboard
			{
				// The point on the ground under the middle of the screen
				constexpr Math::sVector position( 21.0f, 0.0f, -73.0f );
				// The screen is turned around the vertical axis to face the shooting line
				constexpr float yawDegrees = -16.0f;
				// The middle of the screen, above the ground
				constexpr float displayCenterHeight = 7.0f;
				// How far in front of the cabinet the numbers are
				// (in front of the screen's black background and labels, far enough that the depth buffer can always tell them apart)
				constexpr float numbersZ = 0.52f;

				// Positions on the screen are in meters from its middle (x to the right and y up).
				// Each number is placed by its bottom edge and its height (the height of its dots),
				// and by its left or right edge depending on which way it is aligned.
				// The round's total, big, at the right
				constexpr float totalRight = 7.3f;
				constexpr float totalBottom = 0.1f;
				constexpr float totalHeight = 2.6f;
				// The best total so far, at the top right
				constexpr float bestRight = 7.3f;
				constexpr float bestBottom = 2.9f;
				constexpr float bestHeight = 0.7f;
				// How many arrows are left, after its label at the bottom left
				constexpr float arrowsLeftLeft = -0.2f;
				constexpr float arrowsLeftBottom = -3.6f;
				constexpr float arrowsLeftHeight = 0.7f;
				// Each arrow's score, in a row of slots across the middle (each score is centered in its slot)
				constexpr float slotSpacing = 2.45f;
				constexpr float slotBottom = -2.0f;
				constexpr float slotHeight = 1.2f;

				// The newest score flashes while the result is shown, like a stadium screen does
				constexpr float flashPeriodInSeconds = 0.5f;
				constexpr float flashOnFraction = 0.7f;
			}

			// The Quiver
			//===========

			// The ground quiver beside the archer, which holds the arrows that are left
			constexpr Math::sVector quiverPosition( 2.2f, 0.0f, -1.2f );
			// The height of the arrows' nocks above the ground (the arrows stand in the quiver point down)
			constexpr float quiverNockHeight = 1.1f;
			// How far from the middle of the tube each arrow is, and how far it leans out
			constexpr float quiverArrowSpread = 0.03f;
			constexpr float quiverArrowLeanDegrees = 4.0f;

			// The Timing Light
			//=================
			// (Its meshes are built by build_archery_range.py, which has the same position and turn)

			namespace TimingLight
			{
				// The point on the ground under its post, at the side of the shooting line
				constexpr Math::sVector position( -2.2f, 0.0f, -2.0f );
				// Turned around the vertical axis to face the aiming camera
				constexpr float yawDegrees = 28.0f;
			}
			// The referee's whistle blows this long after the arrow is ready to shoot
			// (a moment after the view has settled behind the archer)
			constexpr float readyWhistleDelay = 0.5f;

			// An arrow that hits the target is drawn this far into it (so it looks stuck in the straw rather than touching it)
			constexpr float arrowEmbedDepth = 0.12f;
			// An arrow that hits an obstacle bounces back off it (toward the archer, +z) and falls to the grass
			constexpr float arrowDeflectSpeedBack = 2.0f;
			constexpr float arrowDeflectSpeedUp = 1.2f;
			constexpr float gravity = 9.8f;
			// The height of the arrow's middle when it is lying on the grass
			constexpr float arrowHeightOnGround = 0.03f;

			// Audio
			//======
			// Volumes go from 0 (silent) to 1 (as loud as the sound was made)

			namespace Volume
			{
				// The crowd murmuring, which loops the whole time
				constexpr float ambience = 0.3f;
				constexpr float introFanfare = 0.7f;
				constexpr float bowRelease = 0.9f;
				constexpr float targetHit = 0.9f;
				constexpr float obstacleHit = 0.9f;
				constexpr float bullseye = 0.8f;
				// The cheer gets louder the better the score: cheerMinimum for 1 point up to cheerMaximum for a bullseye
				constexpr float cheerMinimum = 0.45f;
				constexpr float cheerMaximum = 1.0f;
				// Hitting an obstacle gets a bigger groan than just missing
				constexpr float groan_obstacle = 0.8f;
				constexpr float groan_miss = 0.55f;
				constexpr float roundComplete = 0.8f;
				// The whistle is a loud, pure tone, so it is played quietly (it is the referee's, further down the line)
				constexpr float whistle = 0.35f;
			}
			// Skipping the opening shot fades the fanfare out instead of cutting it off
			constexpr float fanfareFadeOutDuration = 0.8f;
			// Each sound that plays often is played slightly higher or lower each time so that it doesn't sound robotic
			// (0.04 means anywhere from 4% lower to 4% higher)
			constexpr float pitchVariation = 0.04f;
		}
	}
}

#endif	// EAE6320_ONESHOT_GAMEPLAYSETTINGS_H
