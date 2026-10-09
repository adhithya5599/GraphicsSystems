/*
	The game's assets (meshes, effects, and textures), each loaded exactly once

	The original game loaded a separate copy of the same effect for every object
	and loaded the obstacle texture once per obstacle into a single pointer
	(so all but the last of those textures could never be released).
	Loading each asset once and letting every game object that uses it add its own reference
	means there is exactly one copy of each asset in memory,
	and it is deleted exactly once, when the last thing using it lets go.
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
			// Not affected by lights (for the sky, which should look the same from every direction)
			Graphics::cEffect* unlitEffect = nullptr;

			// Meshes
			//-------

			Graphics::cMesh* arrowMesh = nullptr;
			Graphics::cMesh* bowMesh = nullptr;
			Graphics::cMesh* groundMesh = nullptr;
			Graphics::cMesh* obstacleMesh = nullptr;
			Graphics::cMesh* skyboxMesh = nullptr;
			Graphics::cMesh* targetMesh = nullptr;

			// Textures
			//---------

			Texture::cTexture* arrowTexture = nullptr;
			Texture::cTexture* bowTexture = nullptr;
			Texture::cTexture* groundTexture = nullptr;
			Texture::cTexture* obstacleTexture = nullptr;
			Texture::cTexture* skyboxTexture = nullptr;
			Texture::cTexture* targetTexture = nullptr;
		};
	}
}

#endif	// EAE6320_ONESHOT_SGAMEASSETS_H
