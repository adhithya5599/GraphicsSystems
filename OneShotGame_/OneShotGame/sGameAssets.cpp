// Includes
//=========

#include "sGameAssets.h"

#include <Engine/Graphics/cEffect.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Texture/cTexture.h>

// Helper Declarations
//====================

namespace
{
	// Each asset type is loaded with the same pattern,
	// so the paths are listed next to the pointer that they are loaded into
	struct sMeshToLoad
	{
		eae6320::Graphics::cMesh*& mesh;
		const char* path;
	};
	struct sTextureToLoad
	{
		eae6320::Texture::cTexture*& texture;
		const char* path;
	};

	template <class tAsset>
	void ReleaseReference( tAsset*& io_asset );
}

// Interface
//==========

eae6320::cResult eae6320::OneShot::sGameAssets::Load()
{
	auto result = Results::Success;

	// Effects
	{
		if ( !( result = Graphics::cEffect::Load( litEffect,
			"data/Shaders/Vertex/standard.shader", "data/Shaders/Fragment/standard.shader" ) ) )
		{
			Logging::OutputError( "The lit effect couldn't be loaded" );
			return result;
		}
		if ( !( result = Graphics::cEffect::Load( unlitEffect,
			"data/Shaders/Vertex/standard.shader", "data/Shaders/Fragment/unlit.shader" ) ) )
		{
			Logging::OutputError( "The unlit effect couldn't be loaded" );
			return result;
		}
	}
	// Meshes
	{
		const sMeshToLoad meshesToLoad[] =
		{
			{ arrowMesh, "data/Meshes/projectile.mayamesh" },
			{ bowMesh, "data/Meshes/player.mayamesh" },
			{ groundMesh, "data/Meshes/ground.mayamesh" },
			{ obstacleMesh, "data/Meshes/obstacle.mayamesh" },
			{ skyboxMesh, "data/Meshes/skybox.mayamesh" },
			{ targetMesh, "data/Meshes/goal.mayamesh" },
		};
		for ( const auto& meshToLoad : meshesToLoad )
		{
			if ( !( result = Graphics::cMesh::Load( meshToLoad.mesh, meshToLoad.path ) ) )
			{
				Logging::OutputError( "The mesh %s couldn't be loaded", meshToLoad.path );
				return result;
			}
		}
	}
	// Textures
	{
		const sTextureToLoad texturesToLoad[] =
		{
			{ arrowTexture, "data/Textures/projectileTexture.bmp" },
			{ bowTexture, "data/Textures/playerTexture.bmp" },
			{ groundTexture, "data/Textures/groundTexture.bmp" },
			{ obstacleTexture, "data/Textures/obstacleTexture.bmp" },
			{ skyboxTexture, "data/Textures/skyboxTexture.bmp" },
			{ targetTexture, "data/Textures/goalTexture.bmp" },
		};
		for ( const auto& textureToLoad : texturesToLoad )
		{
			if ( !( result = Texture::cTexture::Load( textureToLoad.texture, textureToLoad.path ) ) )
			{
				Logging::OutputError( "The texture %s couldn't be loaded", textureToLoad.path );
				return result;
			}
		}
	}

	return result;
}

void eae6320::OneShot::sGameAssets::Release()
{
	ReleaseReference( litEffect );
	ReleaseReference( unlitEffect );

	for ( auto** const mesh : { &arrowMesh, &bowMesh, &groundMesh, &obstacleMesh, &skyboxMesh, &targetMesh } )
	{
		ReleaseReference( *mesh );
	}
	for ( auto** const texture : { &arrowTexture, &bowTexture, &groundTexture, &obstacleTexture, &skyboxTexture, &targetTexture } )
	{
		ReleaseReference( *texture );
	}
}

// Helper Definitions
//===================

namespace
{
	template <class tAsset>
	void ReleaseReference( tAsset*& io_asset )
	{
		if ( io_asset )
		{
			io_asset->DecrementReferenceCount();
			io_asset = nullptr;
		}
	}
}
