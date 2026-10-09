// Includes
//=========

#include "cMyGame.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Graphics/cEffect.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Texture/cTexture.h>
#include <Engine/UserInput/UserInput.h>

// Static Data
//============

namespace
{
	// Both objects use the engine's standard shaders, which are lit by the Graphics system
	// (MyGame doesn't know anything about the lights; they come from Content/Lighting/scene.lighting)
	constexpr auto* const s_standardVertexShaderPath = "data/Shaders/Vertex/standard.shader";
	constexpr auto* const s_standardFragmentShaderPath = "data/Shaders/Fragment/standard.shader";

	// Exported from Content/Blender/BlenderScene.blend with the Blender mesh exporter
	constexpr auto* const s_suzanneMeshPath = "data/Meshes/suzanne.blendermesh";
	constexpr auto* const s_floorMeshPath = "data/Meshes/floor.blendermesh";

	// A white texture leaves the vertex color unchanged (texture * vertex color = vertex color)
	constexpr auto* const s_whiteTexturePath = "data/Textures/whiteTexture.bmp";
	constexpr auto* const s_groundTexturePath = "data/Textures/groundTexture.bmp";
	constexpr auto* const s_waterTexturePath = "data/Textures/waterTexture.bmp";
	constexpr auto* const s_grassTexturePath = "data/Textures/grassTexture.bmp";

	// How fast the arrow keys move the monkey and WASD move the camera (units per second)
	constexpr float s_playerSpeed = 3.0f;
	constexpr float s_cameraSpeed = 3.0f;

	// Returns -1, 0, or 1 depending on which of the two keys is held
	float GetAxis( const uint_fast8_t i_negativeKey, const uint_fast8_t i_positiveKey );
}

// Inherited Implementation
//=========================

// Run
//----

void eae6320::cMyGame::UpdateBasedOnInput()
{
	// Is the user pressing the ESC key?
	if ( UserInput::IsKeyPressed( UserInput::KeyCodes::Escape ) )
	{
		// Exit the application
		const auto result = Exit( EXIT_SUCCESS );
		EAE6320_ASSERT( result );
	}
}

void eae6320::cMyGame::UpdateSimulationBasedOnInput()
{
	m_areTexturesSwapped = UserInput::IsKeyPressed( UserInput::KeyCodes::BackSpace );
	m_suzanne.SetTexture( m_areTexturesSwapped ? m_waterTexture : m_whiteTexture );
	m_floor.SetTexture( m_areTexturesSwapped ? m_grassTexture : m_groundTexture );

	// The arrow keys move the monkey up/down and left/right
	{
		const auto horizontal = GetAxis( UserInput::KeyCodes::Left, UserInput::KeyCodes::Right );
		const auto vertical = GetAxis( UserInput::KeyCodes::Down, UserInput::KeyCodes::Up );
		m_suzanne.GetRigidBodyState().velocity = Math::sVector( horizontal, vertical, 0.0f ) * s_playerSpeed;
	}
	// WASD move the camera forward/back and left/right
	// (the camera looks down -z, so "forward" is -z)
	{
		const auto horizontal = GetAxis( 'A', 'D' );
		const auto forward = GetAxis( 'S', 'W' );
		m_camera.GetRigidBodyState().velocity = Math::sVector( horizontal, 0.0f, -forward ) * s_cameraSpeed;
	}
}

void eae6320::cMyGame::UpdateSimulationBasedOnTime( const float i_elapsedSecondCount_sinceLastUpdate )
{
	m_camera.GetRigidBodyState().Update( i_elapsedSecondCount_sinceLastUpdate );
	m_suzanne.GetRigidBodyState().Update( i_elapsedSecondCount_sinceLastUpdate );
}

void eae6320::cMyGame::SubmitDataToBeRendered( const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	Graphics::SubmitBackgroundColorForANewFrame( 0.5f, 0.0f, 1.0f );
	m_camera.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate, m_aspectRatio );
	m_suzanne.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	m_floor.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::cMyGame::Initialize()
{
	auto result = Results::Success;

	// Load each asset once
	// (each Load() returns a reference that this game owns and releases in CleanUp())
	if ( !( result = Graphics::cEffect::Load( m_effect, s_standardVertexShaderPath, s_standardFragmentShaderPath ) ) )
	{
		EAE6320_ASSERTF( false, "Can't initialize the game without the shading data" );
		return result;
	}
	if ( !( result = Graphics::cMesh::Load( m_suzanneMesh, s_suzanneMeshPath ) )
		|| !( result = Graphics::cMesh::Load( m_floorMesh, s_floorMeshPath ) ) )
	{
		EAE6320_ASSERTF( false, "Can't initialize the game without the geometry data" );
		return result;
	}
	if ( !( result = Texture::cTexture::Load( m_whiteTexture, s_whiteTexturePath ) )
		|| !( result = Texture::cTexture::Load( m_groundTexture, s_groundTexturePath ) )
		|| !( result = Texture::cTexture::Load( m_waterTexture, s_waterTexturePath ) )
		|| !( result = Texture::cTexture::Load( m_grassTexture, s_grassTexturePath ) ) )
	{
		EAE6320_ASSERTF( false, "Can't initialize the game without the texture data" );
		return result;
	}

	// Both objects use the same shaders, so they share the single effect
	// (each game object adds its own reference, so the effect is deleted exactly once:
	// when the game and both objects have all released theirs)
	m_suzanne.SetMesh( m_suzanneMesh );
	m_suzanne.SetEffect( m_effect );
	m_suzanne.SetTexture( m_whiteTexture );
	m_floor.SetMesh( m_floorMesh );
	m_floor.SetEffect( m_effect );
	m_floor.SetTexture( m_groundTexture );

	// The camera starts 10 units in front of the scene, looking at it down -z
	m_camera.GetRigidBodyState().position = Math::sVector( 0.0f, 0.0f, 10.0f );
	m_camera.SetClippingPlanes( 0.1f, 50.0f );
	{
		uint16_t width = 0, height = 0;
		if ( GetCurrentResolution( width, height ) && ( height > 0 ) )
		{
			m_aspectRatio = static_cast<float>( width ) / static_cast<float>( height );
		}
	}

	return result;
}

eae6320::cResult eae6320::cMyGame::CleanUp()
{
	// Everything must be released here rather than in the destructor
	// because this game is destroyed after the Graphics system has shut down
	// (any references that the Graphics system still holds for a frame being rendered
	// are released by Graphics::CleanUp(), which runs after this)
	m_suzanne.ReleaseAssets();
	m_floor.ReleaseAssets();
	if ( m_effect )
	{
		m_effect->DecrementReferenceCount();
		m_effect = nullptr;
	}
	for ( auto** const mesh : { &m_suzanneMesh, &m_floorMesh } )
	{
		if ( *mesh )
		{
			( *mesh )->DecrementReferenceCount();
			*mesh = nullptr;
		}
	}
	for ( auto** const texture : { &m_whiteTexture, &m_groundTexture, &m_waterTexture, &m_grassTexture } )
	{
		if ( *texture )
		{
			( *texture )->DecrementReferenceCount();
			*texture = nullptr;
		}
	}

	return Results::Success;
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
