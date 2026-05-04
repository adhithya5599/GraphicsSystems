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

// Inherited Implementation
//=========================

// Run
//----

bool bIsEnterPressed = false;
bool bIsBackspacePressed = false;

const std::string pixelColorFilePath = "data/Shaders/Vertex/standard.shader";
std::string vertexColorFilePath = "data/Shaders/Fragment/myShader.shader";

const std::string meshGeometryFilePath = "data/Meshes/geometry.lua";
const std::string anotherMeshGeometryFilePath = "data/Meshes/geometryAnother.lua";

//eae6320::Graphics::VertexFormats::sVertex_mesh coordinates[] = {
//	{0.0f, 0.0f, 0.0f},
//	{1.0f, 0.0f, 0.0f},
//	{1.0f, 1.0f, 0.0f},
//	{0.0f, 1.0f, 0.0f}
//};
//
//uint16_t drawOrder[] = {
//	0, 1, 2,
//	2, 3, 0
//};
//
//constexpr auto coordinateCount = sizeof(coordinates) / sizeof(coordinates[0]);
//constexpr auto drawOrderCount = sizeof(drawOrder) / sizeof(drawOrder[0]);
//
//eae6320::Graphics::VertexFormats::sVertex_mesh coordinates1[] = {
//	{1.0f, 1.0f, 1.0f},
//	{0.0f, 1.0f, 1.0f},
//	{0.0f, 0.0f, 1.0f}
//};
//constexpr auto coordinatesCount1 = sizeof(coordinates1) / sizeof(coordinates1[0]);
//
//uint16_t drawOrder1[] = {
//	0, 1, 2,
//};
//constexpr auto drawOrderCount1 = sizeof(drawOrder1) / sizeof(drawOrder1[0]);

eae6320::Graphics::cMesh* drawData[] = { nullptr, nullptr };
eae6320::Graphics::cEffect* effectData[] = { nullptr, nullptr };

eae6320::GameObject::cMyGameObject* playerObject = new eae6320::GameObject::cMyGameObject(drawData[0], effectData[0]);
eae6320::GameObject::cMyGameObject* anotherPlayerObject = new eae6320::GameObject::cMyGameObject(drawData[1], effectData[1]);

eae6320::GameObject::cCamera* camera = new eae6320::GameObject::cCamera();




void eae6320::cMyGame::SubmitDataToBeRendered(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{
	Graphics::SubmitBackgroundColorForANewFrame(1.0f, 1.0f, 1.0f);

	{
		auto& predictedCameraTransform = camera->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
		Graphics::SubmitCameraDataForANewFrame(camera, predictedCameraTransform);
	}

	auto& predictedTransform = playerObject->GetRigidBodyState()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);

	if (bIsBackspacePressed)
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(anotherPlayerObject->GetMesh(), anotherPlayerObject->GetEffect(), predictedTransform);
	}
	else
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(), playerObject->GetEffect(), predictedTransform);
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
	if (!(result = Graphics::cEffect::Load(playerObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(playerObject->GetMesh(), meshGeometryFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}

	vertexColorFilePath = "data/Shaders/Fragment/myAnotherShader.shader";
	if (!(result = Graphics::cEffect::Load(anotherPlayerObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(anotherPlayerObject->GetMesh(), anotherMeshGeometryFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
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

	if (anotherPlayerObject)
	{
		delete anotherPlayerObject;
	}

	if (camera)
	{
		delete camera;
	}

	return Results::Success;
}
