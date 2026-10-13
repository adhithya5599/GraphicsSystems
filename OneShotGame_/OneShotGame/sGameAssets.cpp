// Includes
//=========

#include "sGameAssets.h"

#include <Engine/Audio/cSound.h>
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
	struct sSoundToLoad
	{
		eae6320::Audio::cSound*& sound;
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
			// (The ground is split into several meshes because one mesh can have at most 21,845 triangles;
			// the order must match isRangeMeshGrass)
			{ rangeMeshes[0], "data/Meshes/range_ground_left.blendermesh" },
			{ rangeMeshes[1], "data/Meshes/range_ground_center.blendermesh" },
			{ rangeMeshes[2], "data/Meshes/range_ground_right.blendermesh" },
			{ rangeMeshes[3], "data/Meshes/range_ground_outer.blendermesh" },
			{ rangeMeshes[4], "data/Meshes/range_lines.blendermesh" },
			{ rangeMeshes[5], "data/Meshes/range_stands.blendermesh" },
			{ rangeMeshes[6], "data/Meshes/range_surroundings.blendermesh" },
			{ rangeMeshes[7], "data/Meshes/shooter.blendermesh" },
			{ arrowMesh, "data/Meshes/arrow.blendermesh" },
			{ targetMesh, "data/Meshes/target.blendermesh" },
			{ railSliderFrameMesh, "data/Meshes/railSlider_frame.blendermesh" },
			{ railSliderPlankMesh, "data/Meshes/railSlider_plank.blendermesh" },
			{ bunkerMesh, "data/Meshes/bunker.blendermesh" },
			{ popupBoardMesh, "data/Meshes/popupBoard.blendermesh" },
			{ pulleyFrameMesh, "data/Meshes/pulley_frame.blendermesh" },
			{ pulleyBoardMesh, "data/Meshes/pulley_board.blendermesh" },
			{ ropeMesh, "data/Meshes/rope.blendermesh" },
			{ sledTrackMesh, "data/Meshes/sled_track.blendermesh" },
			{ sledPlankMesh, "data/Meshes/sled_plank.blendermesh" },
			{ flagPoleMesh, "data/Meshes/flagPole.blendermesh" },
			{ flagMesh, "data/Meshes/flag.blendermesh" },
			{ skyDomeMesh, "data/Meshes/skyDome.blendermesh" },
			{ quiverMesh, "data/Meshes/quiver.blendermesh" },
			{ timingLightFrameMesh, "data/Meshes/timingLight_frame.blendermesh" },
			{ timingLightRedMesh, "data/Meshes/timingLight_red.blendermesh" },
			{ timingLightGreenMesh, "data/Meshes/timingLight_green.blendermesh" },
			{ timingLightLabelMesh, "data/Meshes/timingLight_label.blendermesh" },
			{ crowdMeshes[0], "data/Meshes/crowd_0.blendermesh" },
			{ crowdMeshes[1], "data/Meshes/crowd_1.blendermesh" },
			{ crowdMeshes[2], "data/Meshes/crowd_2.blendermesh" },
			{ scoreboardFrameMesh, "data/Meshes/scoreboard_frame.blendermesh" },
			{ scoreboardDisplayMesh, "data/Meshes/scoreboard_display.blendermesh" },
			// (The order must match cScoreboard's glyph indices)
			{ scoreGlyphMeshes[0], "data/Meshes/scoreGlyph_0.blendermesh" },
			{ scoreGlyphMeshes[1], "data/Meshes/scoreGlyph_1.blendermesh" },
			{ scoreGlyphMeshes[2], "data/Meshes/scoreGlyph_2.blendermesh" },
			{ scoreGlyphMeshes[3], "data/Meshes/scoreGlyph_3.blendermesh" },
			{ scoreGlyphMeshes[4], "data/Meshes/scoreGlyph_4.blendermesh" },
			{ scoreGlyphMeshes[5], "data/Meshes/scoreGlyph_5.blendermesh" },
			{ scoreGlyphMeshes[6], "data/Meshes/scoreGlyph_6.blendermesh" },
			{ scoreGlyphMeshes[7], "data/Meshes/scoreGlyph_7.blendermesh" },
			{ scoreGlyphMeshes[8], "data/Meshes/scoreGlyph_8.blendermesh" },
			{ scoreGlyphMeshes[9], "data/Meshes/scoreGlyph_9.blendermesh" },
			{ scoreGlyphMeshes[10], "data/Meshes/scoreGlyph_M.blendermesh" },
			{ scoreGlyphMeshes[11], "data/Meshes/scoreGlyph_dash.blendermesh" },
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
			{ whiteTexture, "data/Textures/white.bmp" },
			{ grassTexture, "data/Textures/grass.bmp" },
			{ woodTexture, "data/Textures/wood.bmp" },
			{ targetTexture, "data/Textures/target.bmp" },
			{ scoreboardTexture, "data/Textures/scoreboard.bmp" },
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
	// Sounds
	{
		const sSoundToLoad soundsToLoad[] =
		{
			{ ambienceSound, "data/Sounds/crowdAmbience.sound" },
			{ introFanfareSound, "data/Sounds/introFanfare.sound" },
			{ bowReleaseSound, "data/Sounds/bowRelease.sound" },
			{ targetHitSound, "data/Sounds/targetHit.sound" },
			{ obstacleHitSound, "data/Sounds/obstacleHit.sound" },
			{ bullseyeSound, "data/Sounds/bullseye.sound" },
			{ cheerSound, "data/Sounds/crowdCheer.sound" },
			{ groanSound, "data/Sounds/crowdGroan.sound" },
			{ roundCompleteSound, "data/Sounds/roundComplete.sound" },
			{ whistleSound, "data/Sounds/whistle.sound" },
		};
		for ( const auto& soundToLoad : soundsToLoad )
		{
			// (A missing sound is logged but doesn't stop the game; see the comment at the top of sGameAssets.h)
			if ( !Audio::cSound::Load( soundToLoad.path, soundToLoad.sound ) )
			{
				Logging::OutputError( "The sound %s couldn't be loaded; the game will play without it", soundToLoad.path );
			}
		}
	}

	return result;
}

void eae6320::OneShot::sGameAssets::Release()
{
	ReleaseReference( litEffect );
	ReleaseReference( unlitEffect );

	for ( auto*& mesh : rangeMeshes )
	{
		ReleaseReference( mesh );
	}
	for ( auto*& mesh : crowdMeshes )
	{
		ReleaseReference( mesh );
	}
	for ( auto*& mesh : scoreGlyphMeshes )
	{
		ReleaseReference( mesh );
	}
	for ( auto** const mesh : { &arrowMesh, &targetMesh, &railSliderFrameMesh, &railSliderPlankMesh, &bunkerMesh, &popupBoardMesh,
		&pulleyFrameMesh, &pulleyBoardMesh, &ropeMesh, &sledTrackMesh, &sledPlankMesh, &flagPoleMesh, &flagMesh, &skyDomeMesh,
		&quiverMesh, &scoreboardFrameMesh, &scoreboardDisplayMesh,
		&timingLightFrameMesh, &timingLightRedMesh, &timingLightGreenMesh, &timingLightLabelMesh } )
	{
		ReleaseReference( *mesh );
	}
	for ( auto** const texture : { &whiteTexture, &grassTexture, &woodTexture, &targetTexture, &scoreboardTexture } )
	{
		ReleaseReference( *texture );
	}
	for ( auto** const sound : { &ambienceSound, &introFanfareSound, &bowReleaseSound, &targetHitSound, &obstacleHitSound,
		&bullseyeSound, &cheerSound, &groanSound, &roundCompleteSound, &whistleSound } )
	{
		ReleaseReference( *sound );
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
