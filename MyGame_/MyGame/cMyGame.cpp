// Includes
//=========

#include "cMyGame.h"

#include <Engine/Graphics/Graphics.h>
#include <Engine/Asserts/Asserts.h>
#include <Engine/UserInput/UserInput.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/cEffect.h>

// Inherited Implementation
//=========================

// Run
//----

bool bIsEnterPressed = false;
bool bIsBackspacePressed = false;

const std::string pixelColorFilePath = "data/Shaders/Vertex/standard.shader";
std::string vertexColorFilePath = "data/Shaders/Fragment/myShader.shader";

eae6320::Graphics::VertexFormats::sVertex_mesh coordinates[] = {
	{0.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 0.0f},
	{1.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 1.0f}
};

uint16_t drawOrder[] = {
	0, 1, 2,
	2, 3, 0
};

constexpr auto coordinateCount = sizeof(coordinates) / sizeof(coordinates[0]);
constexpr auto drawOrderCount = sizeof(drawOrder) / sizeof(drawOrder[0]);

eae6320::Graphics::VertexFormats::sVertex_mesh coordinates1[] = {
	{-1.0f, -1.0f, 1.0f},
	{0.0f, -1.0f, 1.0f},
	{0.0f, 0.0f, 1.0f}
};
constexpr auto coordinatesCount1 = sizeof(coordinates1) / sizeof(coordinates1[0]);

uint16_t drawOrder1[] = {
	0, 1, 2,
};
constexpr auto drawOrderCount1 = sizeof(drawOrder1) / sizeof(drawOrder1[0]);

eae6320::Graphics::cMesh* drawData[] = { nullptr, nullptr };
eae6320::Graphics::cEffect* effectData[] = { nullptr, nullptr };


void eae6320::cMyGame::SubmitDataToBeRendered(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{
	Graphics::SubmitBackgroundColorForANewFrame(1.0f, 0.0f, 1.0f);

	if (!bIsEnterPressed)
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(drawData[0], effectData[0]);
	}
	if (bIsBackspacePressed)
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(drawData[1], effectData[0]);
	}
	else
	{
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(drawData[1], effectData[1]);
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

void eae6320::cMyGame::UpdateSimulationBasedOnInput()
{
	//cMyGame myGameObject;
	//Is the user pressed the Enter key?
	if (UserInput::IsKeyPressed(UserInput::KeyCodes::Enter))
		bIsEnterPressed = true;
	else
		bIsEnterPressed = false;
	if (UserInput::IsKeyPressed(UserInput::KeyCodes::BackSpace))
		bIsBackspacePressed = true;
	else
		bIsBackspacePressed = false;
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::cMyGame::Initialize()
{
	auto result = Results::Success;

	if (!(result = Graphics::cEffect::Load(effectData[0], pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(drawData[0], drawOrderCount, coordinates, coordinateCount, drawOrder)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}
	
	vertexColorFilePath = "data/Shaders/Fragment/myAnotherShader.shader";
	if (!(result = Graphics::cEffect::Load(effectData[1], pixelColorFilePath, vertexColorFilePath)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
		return result;
	}
	if (!(result = Graphics::cMesh::Load(drawData[1], drawOrderCount1, coordinates1, coordinatesCount1, drawOrder1)))
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
		return result;
	}
	return result;
}

eae6320::cResult eae6320::cMyGame::CleanUp()
{
	if (effectData[0])
	{
		effectData[0]->DecrementReferenceCount();
		effectData[0] = nullptr;
	}
	if (drawData[0])
	{
		drawData[0]->DecrementReferenceCount();
		drawData[0] = nullptr;
	}
	if (effectData[1])
	{
		effectData[1]->DecrementReferenceCount();
		effectData[1] = nullptr;
	}
	if (drawData[1])
	{
		drawData[1]->DecrementReferenceCount();
		drawData[1] = nullptr;
	}
	return Results::Success;
}
