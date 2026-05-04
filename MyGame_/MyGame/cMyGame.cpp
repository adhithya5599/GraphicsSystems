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

const std::string pixelColorFilePath = "data/Shaders/Vertex/standard.shader";
std::string vertexColorFilePath = "data/Shaders/Fragment/myShader.shader";

//const std::string meshGeometryFilePath = "data/Meshes/geometry.lua";
//const std::string anotherMeshGeometryFilePath = "data/Meshes/geometryAnother.lua";

const std::string planeMayaMesh = "data/Meshes/plane.mayamesh";
const std::string cylinderMayaMesh = "data/Meshes/cylinder.mayamesh";
const std::string primsMayaMesh = "data/Meshes/prism.mayamesh";

const std::string textureFilePath = "data/Textures/groundTexture.bmp";
const std::string defaultTexturePath = "data/Textures/whiteTexture.bmp";
const std::string grassTexturePath = "data/Textures/grassTexture.bmp";
const std::string waterTexturePath = "data/Textures/waterTexture.bmp";
const std::string lensTexturePath = "data/Textures/lensTexture.bmp";

eae6320::Graphics::cMesh* drawData[] = { nullptr, nullptr, nullptr };
eae6320::Graphics::cEffect* effectData[] = { nullptr, nullptr, nullptr };

eae6320::Texture::cTexture* groundTexture = nullptr;
eae6320::Texture::cTexture* noTexture = nullptr;
eae6320::Texture::cTexture* waterTexture = nullptr;
eae6320::Texture::cTexture* grassTexture = nullptr;
eae6320::Texture::cTexture* lensTexture = nullptr;

eae6320::GameObject::cMyGameObject* playerObject = new eae6320::GameObject::cMyGameObject(drawData[0], effectData[0]);
eae6320::GameObject::cMyGameObject* planeObject = new eae6320::GameObject::cMyGameObject(drawData[1], effectData[1]);
eae6320::GameObject::cMyGameObject* anotherObject = new eae6320::GameObject::cMyGameObject(drawData[2], effectData[2]);

eae6320::GameObject::cCamera* camera = new eae6320::GameObject::cCamera();

void eae6320::cMyGame::SubmitDataToBeRendered(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{
	Graphics::SubmitBackgroundColorForANewFrame(0.5f, 0.0f, 1.0f);

	{
		auto& predictedCameraTransform = camera->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
		Graphics::SubmitCameraDataForANewFrame(camera, predictedCameraTransform);
	}

	auto& predictedTransform = playerObject->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
	auto& planeTransform = planeObject->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
	auto& anotherTransform = anotherObject->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);

	if (bIsBackspacePressed)
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(),
			planeObject->GetEffect(), predictedTransform, waterTexture, 0);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(planeObject->GetMesh(),
			planeObject->GetEffect(), planeTransform, grassTexture, 0);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(anotherObject->GetMesh(),
			planeObject->GetEffect(), anotherTransform, lensTexture, 0);
	}
	else
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(), 
			playerObject->GetEffect(), predictedTransform, noTexture, 0);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(planeObject->GetMesh(), 
			planeObject->GetEffect(), planeTransform, groundTexture, 0);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(anotherObject->GetMesh(), 
			anotherObject->GetEffect(), anotherTransform, noTexture, 0);
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
		playerObject->GetRigidBodyState()->velocity = { 0.0f, 10.0f, 0.0f };
	
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::Down))
		playerObject->GetRigidBodyState()->velocity = { 0.0f, -10.f, 0.0f };
	
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::Left))
		playerObject->GetRigidBodyState()->velocity = { -10.0f, 0.0f, 0.0f };
	
	else if (UserInput::IsKeyPressed(UserInput::KeyCodes::Right))
		playerObject->GetRigidBodyState()->velocity = { 10.0f, 0.f, 0.0f };
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
	
	vertexColorFilePath = "data/Shaders/Fragment/standard.shader";
	if (!(result = Graphics::cEffect::Load(playerObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(playerObject->GetMesh(), cylinderMayaMesh)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}

	vertexColorFilePath = "data/Shaders/Fragment/standard.shader";
	if (!(result = Graphics::cEffect::Load(planeObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(planeObject->GetMesh(), planeMayaMesh)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}

	vertexColorFilePath = "data/Shaders/Fragment/myShader.shader";
	if (!(result = Graphics::cEffect::Load(anotherObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(anotherObject->GetMesh(), primsMayaMesh)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}

	if (!(result = Texture::cTexture::Load(groundTexture, textureFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the texture data");
		return result;
	}
	if (!(result = Texture::cTexture::Load(grassTexture, grassTexturePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the texture data");
		return result;
	}
	if (!(result = Texture::cTexture::Load(lensTexture, lensTexturePath)))
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
	if (playerObject)
	{
		delete playerObject;
	}

	if (planeObject)
	{
		delete planeObject;
	}

	if (anotherObject)
	{
		delete anotherObject;
	}

	if (camera)
	{
		delete camera;
	}

	return Results::Success;
}
