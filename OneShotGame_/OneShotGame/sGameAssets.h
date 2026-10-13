/*
	The game's assets (meshes, effects, textures, and sounds), each loaded exactly once

	Loading each asset once and letting every game object that uses it add its own reference
	means there is exactly one copy of each asset in memory (e.g. one target mesh for all five targets),
	and it is deleted exactly once, when the last thing using it lets go.

	The meshes and textures are made by OneShotGame_/Content/Blender/build_archery_range.py and make_textures.py,
	and the sounds by OneShotGame_/Content/Sounds/make_sounds.py.

	A sound that can't be loaded isn't an error that stops the game (like a missing mesh would be):
	its pointer stays null and the game just doesn't play it, the same way it would play with no audio device.
*/

#ifndef EAE6320_ONESHOT_SGAMEASSETS_H
#define EAE6320_ONESHOT_SGAMEASSETS_H

// Includes
//=========

#include <Engine/Results/Results.h>

// Forward Declarations
//=====================

namespace eae6320
{
	namespace Audio
	{
		class cSound;
	}

	namespace Graphics
	{
		class cEffect;
		class cMesh;
	}

	namespace Texture
	{
		class cTexture;
	}
}

// Struct Declaration
//===================

namespace eae6320
{
	namespace OneShot
	{
		struct sGameAssets
		{
			// Interface
			//==========

			cResult Load();
			// Releases this object's reference to every asset
			// (it must be called before the Graphics system shuts down)
			void Release();

			~sGameAssets() { Release(); }

			// Data
			//=====

			// Effects
			//--------

			// Lit by the scene's lights (see Content/Lighting/scene.lighting)
			Graphics::cEffect* litEffect = nullptr;
			// Not affected by lights (for the sky, which is the light source rather than something being lit,
			// and for the big screen, which gives off its own light)
			Graphics::cEffect* unlitEffect = nullptr;

			// Meshes
			//-------

			// The range itself, which never moves (modeled in world space)
			static constexpr unsigned int rangeMeshCount = 8;
			Graphics::cMesh* rangeMeshes[rangeMeshCount] = {};
			// For each range mesh, whether it uses the grass texture (the others use the white texture)
			static constexpr bool isRangeMeshGrass[rangeMeshCount] = { true, true, true, true, false, false, false, false };

			// Things that the game places and moves (each modeled around its own origin)
			Graphics::cMesh* arrowMesh = nullptr;
			Graphics::cMesh* targetMesh = nullptr;
			Graphics::cMesh* railSliderFrameMesh = nullptr;
			Graphics::cMesh* railSliderPlankMesh = nullptr;
			Graphics::cMesh* bunkerMesh = nullptr;
			Graphics::cMesh* popupBoardMesh = nullptr;
			Graphics::cMesh* pulleyFrameMesh = nullptr;
			Graphics::cMesh* pulleyBoardMesh = nullptr;
			Graphics::cMesh* ropeMesh = nullptr;
			Graphics::cMesh* sledTrackMesh = nullptr;
			Graphics::cMesh* sledPlankMesh = nullptr;
			Graphics::cMesh* flagPoleMesh = nullptr;
			Graphics::cMesh* flagMesh = nullptr;
			Graphics::cMesh* skyDomeMesh = nullptr;
			Graphics::cMesh* quiverMesh = nullptr;
			// The timing light at the shooting line: its housing (lit), its red and green lamps, and the word "SPACE" (unlit)
			Graphics::cMesh* timingLightFrameMesh = nullptr;
			Graphics::cMesh* timingLightRedMesh = nullptr;
			Graphics::cMesh* timingLightGreenMesh = nullptr;
			Graphics::cMesh* timingLightLabelMesh = nullptr;

			// The crowd is split into groups so that each group can hop up and down on its own when they cheer
			// (modeled in world space, like the range)
			static constexpr unsigned int crowdGroupCount = 3;
			Graphics::cMesh* crowdMeshes[crowdGroupCount] = {};

			// The big screen: its cabinet (lit), its display (unlit: the black background and the labels),
			// and one small quad for each character that a number can use
			Graphics::cMesh* scoreboardFrameMesh = nullptr;
			Graphics::cMesh* scoreboardDisplayMesh = nullptr;
			// "0" to "9", then "M" (a miss), then "-" (no score yet); see cScoreboard
			static constexpr unsigned int scoreGlyphCount = 12;
			Graphics::cMesh* scoreGlyphMeshes[scoreGlyphCount] = {};

			// Textures
			//---------

			// Most meshes are colored by their vertex colors alone (texture * vertex color = vertex color for a white texture)
			Texture::cTexture* whiteTexture = nullptr;
			Texture::cTexture* grassTexture = nullptr;
			Texture::cTexture* woodTexture = nullptr;
			Texture::cTexture* targetTexture = nullptr;
			// The big screen's characters and labels (white LED dots on black), also used by the timing light
			Texture::cTexture* scoreboardTexture = nullptr;

			// Sounds
			//-------

			// The crowd murmuring (loops the whole time)
			Audio::cSound* ambienceSound = nullptr;
			// Plays during the opening shot
			Audio::cSound* introFanfareSound = nullptr;
			Audio::cSound* bowReleaseSound = nullptr;
			Audio::cSound* targetHitSound = nullptr;
			Audio::cSound* obstacleHitSound = nullptr;
			Audio::cSound* bullseyeSound = nullptr;
			Audio::cSound* cheerSound = nullptr;
			Audio::cSound* groanSound = nullptr;
			Audio::cSound* roundCompleteSound = nullptr;
			// The referee's whistle that means the archer may shoot
			Audio::cSound* whistleSound = nullptr;
		};
	}
}

#endif	// EAE6320_ONESHOT_SGAMEASSETS_H
