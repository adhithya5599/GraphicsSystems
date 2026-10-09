// Includes
//=========

#include "cMyGame.h"

#include <Engine/Graphics/Graphics.h>
#include <Engine/Asserts/Asserts.h>
#include <Engine/UserInput/UserInput.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/cEffect.h>
#include <Engine/GameObject/cGameObject.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Physics/sRigidBodyState.h>
#include <Engine/GameObject/cCamera.h>
#include <Engine/Texture/cTexture.h>

// Inherited Implementation
//=========================

// Run
//----

bool bIsEnterPressed = false;
bool bIsBackspacePressed = false;

// Both objects use the engine's standard shaders, which are lit by the Graphics system
// (MyGame doesn't know anything about the lights; they come from Content/Lighting/scene.lighting)
const std::string standardVertexShaderPath = "data/Shaders/Vertex/standard.shader";
const std::string standardFragmentShaderPath = "data/Shaders/Fragment/standard.shader";

// Exported from Content/Blender/BlenderScene.blend with the Blender mesh exporter
const std::string suzanneBlenderMesh = "data/Meshes/suzanne.blendermesh";
const std::string floorBlenderMesh = "data/Meshes/floor.blendermesh";

const std::string groundTexturePath = "data/Textures/groundTexture.bmp";
const std::string defaultTexturePath = "data/Textures/whiteTexture.bmp";
const std::string grassTexturePath = "data/Textures/grassTexture.bmp";
const std::string waterTexturePath = "data/Textures/waterTexture.bmp";

// How fast the arrow keys move the monkey (units per second)
constexpr float s_playerSpeed = 3.0f;

eae6320::Graphics::cMesh* drawData[] = { nullptr, nullptr };
eae6320::Graphics::cEffect* effectData[] = { nullptr, nullptr };

eae6320::Texture::cTexture* groundTexture = nullptr;
// A white texture leaves the vertex color unchanged (texture * vertex color = vertex color)
eae6320::Texture::cTexture* noTexture = nullptr;
eae6320::Texture::cTexture* waterTexture = nullptr;
eae6320::Texture::cTexture* grassTexture = nullptr;

// The monkey is the "player" that the arrow keys move;
// moving it around the fixed lights is an easy way to see the lighting change
eae6320::GameObject::cMyGameObject* playerObject = new eae6320::GameObject::cMyGameObject(drawData[0], effectData[0]);
eae6320::GameObject::cMyGameObject* floorObject = new eae6320::GameObject::cMyGameObject(drawData[1], effectData[1]);

eae6320::GameObject::cCamera* camera = new eae6320::GameObject::cCamera();

void eae6320::cMyGame::SubmitDataToBeRendered(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{
	Graphics::SubmitBackgroundColorForANewFrame(0.5f, 0.0f, 1.0f);

	{
		auto predictedCameraTransform = camera->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
		Graphics::SubmitCameraDataForANewFrame(camera, predictedCameraTransform);
	}

	// (These are copies rather than references because PredictFutureTransform() returns a temporary)
	auto playerTransform = playerObject->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
	auto floorTransform = floorObject->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);

	// Holding Backspace swaps the textures
	if (bIsBackspacePressed)
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(),
			playerObject->GetEffect(), playerTransform, waterTexture, 0);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(floorObject->GetMesh(),
			floorObject->GetEffect(), floorTransform, grassTexture, 0);
	}
	else
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(),
			playerObject->GetEffect(), playerTransform, noTexture, 0);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(floorObject->GetMesh(),
			floorObject->GetEffect(), floorTransform, groundTexture, 0);
	}
}

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

void eae6320::cMyGame::UpdateSimulationBasedOnTime(const float i_elapsedSecondCount_sinceLastUpdate)
{
	camera->GetRigidBodyState()->Update(i_elapsedSecondCount_sinceLastUpdate);

	playerObject->GetRigidBodyState()->Update(i_elapsedSecondCount_sinceLastUpdate);
}

void eae6320::cMyGame::UpdateSimulationBasedOnInput()
{
	if (UserInput::IsKeyPressed(UserInput::KeyCodes::BackSpace))
		bIsBackspacePressed = true;
	else
		bIsBackspacePressed = false;

	if (UserInput::IsKeyPressed(UserInput::KeyCodes::Up))
		playerObject->GetRigidBodyState()->velocity = { 0.0f, s_playerSpeed, 0.0f };
	
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::Down))
		playerObject->GetRigidBodyState()->velocity = { 0.0f, -s_playerSpeed, 0.0f };
	
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::Left))
		playerObject->GetRigidBodyState()->velocity = { -s_playerSpeed, 0.0f, 0.0f };
	
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::Right))
		playerObject->GetRigidBodyState()->velocity = { s_playerSpeed, 0.0f, 0.0f };
	else
		playerObject->GetRigidBodyState()->velocity = { 0.0f, 0.0f, 0.0f };

	if (UserInput::IsKeyPressed(UserInput::KeyCodes::W))
		camera->GetRigidBodyState()->velocity = {0.0f, -1.0f, 0.f};
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::A))
		camera->GetRigidBodyState()->velocity = { 1.0f, 0.f, 0.f };
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::S))
		camera->GetRigidBodyState()->velocity = { 0.0f, 1.f, 0.f };
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::D))
		camera->GetRigidBodyState()->velocity = { -1.0f, 0.f, 0.f };
	else
		camera->GetRigidBodyState()->velocity = {0.f, 0.f, 0.f};

}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::cMyGame::Initialize()
{
	auto result = Results::Success;

	// Both objects use the same shaders, so they share a single effect instead of loading it twice.
	// The effect is reference counted:
	// Load() returns it with a count of 1 (owned by the player),
	// IncrementReferenceCount() adds a reference for the floor,
	// and each game object's destructor releases its reference,
	// so the effect is deleted exactly once, when the last object using it is destroyed.
	if (!(result = Graphics::cEffect::Load(playerObject->GetEffect(), standardVertexShaderPath, standardFragmentShaderPath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	floorObject->GetEffect() = playerObject->GetEffect();
	floorObject->GetEffect()->IncrementReferenceCount();

	if (!(result = Graphics::cMesh::Load(playerObject->GetMesh(), suzanneBlenderMesh)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(floorObject->GetMesh(), floorBlenderMesh)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}

	if (!(result = Texture::cTexture::Load(groundTexture, groundTexturePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the texture data");
		return result;
	}
	if (!(result = Texture::cTexture::Load(grassTexture, grassTexturePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the texture data");
		return result;
	}
	if (!(result = Texture::cTexture::Load(noTexture, defaultTexturePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the texture data");
		return result;
	}
	if (!(result = Texture::cTexture::Load(waterTexture, waterTexturePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the texture data");
		return result;
	}

	return result;
}

eae6320::cResult eae6320::cMyGame::CleanUp()
{
	// Deleting a game object releases its references to its mesh and effect
	// (any references that the Graphics system still holds for a frame being rendered
	// are released by Graphics::CleanUp(), which runs after this)
	if (playerObject)
	{
		delete playerObject;
		playerObject = nullptr;
	}

	if (floorObject)
	{
		delete floorObject;
		floorObject = nullptr;
	}

	if (camera)
	{
		delete camera;
		camera = nullptr;
	}

	// The textures were loaded by this game, so this game must release its references
	// (they used to be leaked: each Load() returns a reference that has to be released)
	for (auto** const texture : { &groundTexture, &noTexture, &waterTexture, &grassTexture })
	{
		if (*texture)
		{
			(*texture)->DecrementReferenceCount();
			*texture = nullptr;
		}
	}

	return Results::Success;
}
