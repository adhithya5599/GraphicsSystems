// Includes
//=========

#include "cOneShotGame.h"

#include <Engine/Graphics/Graphics.h>
#include <Engine/Asserts/Asserts.h>
#include <Engine/UserInput/UserInput.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/cEffect.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Texture/cTexture.h>

//New Includes
#include <Engine/CameraControls/cTrackingCamera.h>
#include <Engine/Physics/cPhysicsBody2D.h>
#include <Engine/Math/sVector2.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <Engine/Platform/Platform.h>

// Inherited Implementation
//=========================

// Run
//----

//Cameras
//----------------
eae6320::Camera::cTrackingCamera firstPersonCamera;
const std::string firstPersonCameraFilePath = "data/Cameras/firstPersonCamera.camera";
float aspectRatio = 1.0f;

eae6320::Camera::cTrackingCamera thirdPersonCamera;
const std::string thirdPersonCameraFilePath = "data/Cameras/thirdPersonFollowCamera.camera";
//----------------

const std::string pixelColorFilePath = "data/Shaders/Vertex/standard.shader";
std::string vertexColorFilePath = "data/Shaders/Fragment/standard.shader";

//Ground Object
//----------------
const std::string groundMayaMeshFilePath = "data/Meshes/ground.mayamesh";
const std::string groundTextureFilePath = "data/Textures/groundTexture.bmp";

eae6320::Graphics::cEffect* groundEffect = nullptr;
eae6320::Graphics::cMesh* groundMesh = nullptr;
eae6320::Texture::cTexture* groundTexture = nullptr;

eae6320::GameObject::cMyGameObject* groundObject = new eae6320::GameObject::cMyGameObject(groundMesh, groundEffect);
//----------------

//skybox Object
//----------------
const std::string skyboxMayaMeshFilePath = "data/Meshes/skybox.mayamesh";
const std::string skyboxTextureFilePath = "data/Textures/skyboxTexture.bmp";

eae6320::Graphics::cEffect* skyboxEffect = nullptr;
eae6320::Graphics::cMesh* skyboxMesh = nullptr;
eae6320::Texture::cTexture* skyboxTexture = nullptr;

eae6320::GameObject::cMyGameObject* skyboxObject = new eae6320::GameObject::cMyGameObject(skyboxMesh, skyboxEffect);
//----------------

//Player Object
//----------------
const std::string playerMayaMeshFilePath = "data/Meshes/player.mayamesh";
const std::string playerTextureFilePath = "data/Textures/playerTexture.bmp";

eae6320::Graphics::cEffect* playerEffect = nullptr;
eae6320::Graphics::cMesh* playerMesh = nullptr;
eae6320::Texture::cTexture* playerTexture = nullptr;

eae6320::GameObject::cMyGameObject* playerObject = new eae6320::GameObject::cMyGameObject(playerMesh, playerEffect);
//----------------

//Projectile Object
//----------------
const std::string projectileMayaMeshFilePath = "data/Meshes/projectile.mayamesh";
const std::string projectileTextureFilePath = "data/Textures/projectileTexture.bmp";

eae6320::Graphics::cEffect* projectileEffect = nullptr;
eae6320::Graphics::cMesh* projectileMesh = nullptr;
eae6320::Texture::cTexture* projectileTexture = nullptr;

eae6320::GameObject::cMyGameObject* projectileObject = new eae6320::GameObject::cMyGameObject(projectileMesh, projectileEffect);

bool isProjectileActive = false;
//----------------

//Obstacle Object
//----------------
const std::string obstacleMayaMeshFilePath = "data/Meshes/obstacle.mayamesh";
const std::string obstacleTextureFilePath = "data/Textures/obstacleTexture.bmp";

eae6320::Graphics::cMesh* obstacleMesh = nullptr;
eae6320::Graphics::cEffect* obstacleEffect = nullptr;
eae6320::Texture::cTexture* obstacleTexture = nullptr;
//----------------

//Goal Object
//----------------
const std::string goalMayaMeshFilePath = "data/Meshes/goal.mayamesh";
const std::string goalTextureFilePath = "data/Textures/goalTexture.bmp";

eae6320::Graphics::cEffect* goalEffect = nullptr;
eae6320::Graphics::cMesh* goalMesh = nullptr;
eae6320::Texture::cTexture* goalTexture = nullptr;

eae6320::GameObject::cMyGameObject* goalObject = new eae6320::GameObject::cMyGameObject(goalMesh, goalEffect);
//----------------

eae6320::Math::sVector playerStartPosition = eae6320::Math::sVector(0.f, 5.f, 0.f);
eae6320::Math::cQuaternion playerStartOrientation = eae6320::Math::cQuaternion();

eae6320::Math::sVector projectileStartPosition = eae6320::Math::sVector(0.f, -5.f, 0.f);
eae6320::Math::cQuaternion projectileStartOrientation = eae6320::Math::cQuaternion();

//Player controls
//----------------
float projectileSpeedXY = 2.5f;
float projectileBoostSpeed = 10.0f;
float projectileNormalSpeed = 5.0f;
//----------------

//Obstacle Positions
//----------------
static const float obstacleZPositions[] = { -12.f, -10.0f, -17.0f, -25.0f, -27.0f};
static const eae6320::Math::sVector2 obstacleXYPositions[] = {
	eae6320::Math::sVector2(-3.f, 1.5f),
	eae6320::Math::sVector2(-1.5f, 2.f),
	eae6320::Math::sVector2(0.f, 1.f),
	eae6320::Math::sVector2(1.5f, 2.5f),
	eae6320::Math::sVector2(3.f, 1.5f)
};
//----------------

//Projectile Collision
//----------------
float projectilePreviousZ = 0.0f;
//----------------

void eae6320::cOneShotGame::UpdateBasedOnInput()
{
	if (m_isGameEnded)
	{
		// Exit the application
		const auto result = Exit(EXIT_SUCCESS);
		EAE6320_ASSERT(result);
	}

	// Is the user pressing the ESC key?
	if ( UserInput::IsKeyPressed( UserInput::KeyCodes::Escape ))
	{
		// Exit the application
		const auto result = Exit( EXIT_SUCCESS );
		EAE6320_ASSERT( result );
	}
}

void eae6320::cOneShotGame::SubmitDataToBeRendered(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{ 
	//Submit Render data for Camera
	{
		if (m_CameraMode == eCameraMode::FirstPerson)
		{
			firstPersonCamera.SubmitRenderData(firstPersonCamera.GetPosition(), firstPersonCamera.GetOrientation(), aspectRatio);
			Math::cMatrix_transformation skyboxTransform = Math::cMatrix_transformation(Math::cQuaternion(), firstPersonCamera.GetPosition());
			Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(skyboxObject->GetMesh(), skyboxObject->GetEffect(),
				skyboxTransform, skyboxTexture, 0);
		}
		else
		{
			thirdPersonCamera.SubmitRenderData(thirdPersonCamera.GetPosition(), thirdPersonCamera.GetOrientation(), aspectRatio);
			Math::cMatrix_transformation skyboxTransform = Math::cMatrix_transformation(Math::cQuaternion(), thirdPersonCamera.GetPosition());
			Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(skyboxObject->GetMesh(), skyboxObject->GetEffect(),
				skyboxTransform, skyboxTexture, 0);
		}
	}
	//Submit Render data for Ground
	{
		auto groundTransform = groundObject->GetPhysicsBody2D()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(groundObject->GetMesh(), groundObject->GetEffect(),
			groundTransform, groundTexture, 0);
	}

	//Submit Render data for Goal
	{
		auto goalTransform = goalObject->GetPhysicsBody2D()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
		goalTransform.SetLocation(eae6320::Math::sVector(goalTransform.GetLocation().x,
			goalTransform.GetLocation().y, m_goalZPosition));
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(goalObject->GetMesh(), goalObject->GetEffect(),
			goalTransform, goalTexture, 0);
	}

	//Submit Render data for Obstacles
	{
		for(const auto& obstacle : m_Obstacles)
		{
			auto obstacleTransform = obstacle.object->GetPhysicsBody2D()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
			obstacleTransform.SetLocation(eae6320::Math::sVector(obstacleTransform.GetLocation().x,
				obstacleTransform.GetLocation().y, obstacle.zPosition));
			Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(obstacle.object->GetMesh(), obstacle.object->GetEffect(),
				obstacleTransform, obstacleTexture, 0);
		}
	}

	if(isProjectileActive)
	{
		auto playerTransform = playerObject->GetPhysicsBody2D()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(), playerObject->GetEffect(),
			playerTransform, playerTexture, 0);

		//Submit Render data for Projectile
		eae6320::Math::cMatrix_transformation projectileTransform(eae6320::Math::cQuaternion(),
			eae6320::Math::sVector(projectileObject->GetPhysicsBody2D()->position.x, 
			projectileObject->GetPhysicsBody2D()->position.y, m_ProjectileZ));
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(projectileObject->GetMesh(), projectileObject->GetEffect(),
			projectileTransform, projectileTexture, 0);
	}
	else
	{
		//Submit Render data for Player
		{
			auto playerTransform = playerObject->GetPhysicsBody2D()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
			Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(playerObject->GetMesh(), playerObject->GetEffect(),
				playerTransform, playerTexture, 0);
			auto projectileTransform = projectileObject->GetPhysicsBody2D()->PredictFutureTransform(i_elapsedSecondCount_sinceLastSimulationUpdate);
			Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(projectileObject->GetMesh(), projectileObject->GetEffect(),
				projectileTransform, projectileTexture, 0);
		}
	}

}

void eae6320::cOneShotGame::UpdateSimulationBasedOnInput()
{
	//Pause Game Input
	if( UserInput::IsKeyPressed( UserInput::KeyCodes::P ))
	{
		m_isGamePaused = !m_isGamePaused;
	}

	//Switch to First Person Camera
	if(UserInput::IsKeyPressed( UserInput::KeyCodes::F ))
	{
		m_CameraMode = eCameraMode::FirstPerson;
	}

	//Slow time
	if(UserInput::IsKeyPressed( UserInput::KeyCodes::O ) && !UserInput::IsKeyPressed( UserInput::KeyCodes::Shift ))
	{
		m_isTimeSlowed = !m_isTimeSlowed;
	}

	if(UserInput::IsKeyPressed( UserInput::KeyCodes::Space ))
	{
		if(!isProjectileActive)
		{
			isProjectileActive = true;
			m_CameraMode = eCameraMode::FirstPerson;
			const auto& currentProjectilePosition = projectileObject->GetPhysicsBody2D()->position;
			constexpr eae6320::Math::sVector projectileOffsetFromPlayer = eae6320::Math::sVector(0.f, 1.5f, 0.f);
			m_ProjectileZ = 0.0f;
			projectileStartPosition = eae6320::Math::sVector(currentProjectilePosition.x, currentProjectilePosition.y, m_ProjectileZ)
				+ projectileOffsetFromPlayer;

			//Give the projectile an initial velocity
			projectileObject->GetPhysicsBody2D()->velocity = eae6320::Math::sVector2(0.f, 0.0f);
			m_ProjectileZVelocity = -projectileNormalSpeed;
		}
	}

	if (isProjectileActive)
	{
		auto& projectilePhysicsBody = projectileObject->GetPhysicsBody2D();
		if(!projectilePhysicsBody)
		{
			return;
		}

		eae6320::Math::sVector2 zeroVelocity = eae6320::Math::sVector2(0.f, 0.f);
		
		//Move projectile Up and down
		if (UserInput::IsKeyPressed(UserInput::KeyCodes::W))
		{
			zeroVelocity.y += projectileSpeedXY;
		}
		if (UserInput::IsKeyPressed(UserInput::KeyCodes::S))
		{
			zeroVelocity.y -= projectileSpeedXY;
		}
		
		if (UserInput::IsKeyPressed(UserInput::KeyCodes::A))
		{
			zeroVelocity.x -= projectileSpeedXY;
		}
		if (UserInput::IsKeyPressed(UserInput::KeyCodes::D))
		{
			zeroVelocity.x += projectileSpeedXY;
		}

		projectilePhysicsBody->velocity = zeroVelocity;

		//Boost projectile speed
		if (UserInput::IsKeyPressed(UserInput::KeyCodes::Shift) && !m_isTimeSlowed)
		{
			m_ProjectileZVelocity = -projectileBoostSpeed;
		}
		else
		{
			m_ProjectileZVelocity = -projectileNormalSpeed;
		}
	}
}

void eae6320::cOneShotGame::UpdateSimulationBasedOnTime(float i_elapsedSecondCount_sinceLastUpdate)
{
	//If game is paused, skip the update
	if (m_isGamePaused)
	{
		return;
	}

	if (m_isTimeSlowed)
	{
		i_elapsedSecondCount_sinceLastUpdate *= 0.1f;
	}

	//Update the Projectile Physics
	const auto physicsResult = m_PhysicsWorld.get()->BodyCount();
	for (int i = 0; i < physicsResult; ++i)
	{
		Physics::PhysicsBody2D* body = nullptr;
		if (m_PhysicsWorld.get()->GetBody(i, body))
		{
			body->Update(i_elapsedSecondCount_sinceLastUpdate, i_elapsedSecondCount_sinceLastUpdate);
		}
	}

	if (CheckProjectileObstacleCollision())
	{		
		MarkGameAsEnded();
		m_CameraMode = eCameraMode::ThirdPerson2D;
	}

	if (CheckProjectileGoalCollision())
	{		
		MarkGameAsEnded();
		m_CameraMode = eCameraMode::ThirdPerson2D;
	}
	
	if(m_CameraMode == eCameraMode::FirstPerson)
	{
		auto& currentProjectilePosition = projectileObject->GetPhysicsBody2D()->position;
		currentProjectilePosition.x = std::max(-8.f, std::min(8.f, currentProjectilePosition.x));
		currentProjectilePosition.y = std::max(0.f, std::min(6.f, currentProjectilePosition.y));

		constexpr eae6320::Math::sVector projectileOffsetFromPlayer = eae6320::Math::sVector(0.f, 0.2f, 0.75f);
		projectilePreviousZ = m_ProjectileZ ;
		m_ProjectileZ += m_ProjectileZVelocity * i_elapsedSecondCount_sinceLastUpdate;
		projectileStartPosition = eae6320::Math::sVector(currentProjectilePosition.x, currentProjectilePosition.y, m_ProjectileZ)
			+ projectileOffsetFromPlayer;
		firstPersonCamera.Update(i_elapsedSecondCount_sinceLastUpdate);

		//Deactivate projectile if it goes out of bounds
		if (currentProjectilePosition.y > 20.f || currentProjectilePosition.y < -20.f ||
			currentProjectilePosition.x > 20.f || currentProjectilePosition.x < -20.f)
		{
			isProjectileActive = false;
		}
	}
	else
	{
		const auto& currentPlayerPosition = playerObject->GetPhysicsBody2D()->position;
		playerStartPosition = eae6320::Math::sVector(currentPlayerPosition.x, currentPlayerPosition.y + 2.0f, 12.f);
		thirdPersonCamera.Update(i_elapsedSecondCount_sinceLastUpdate);
	}

	m_obstacleTime += i_elapsedSecondCount_sinceLastUpdate;
	for (auto& obstacle : m_Obstacles)
	{
		const float offset = std::sinf(m_obstacleTime * obstacle.frequency) * obstacle.amplitude;
		auto* obstacleBody = obstacle.object->GetPhysicsBody2D();
		eae6320::Math::sVector2 newPosition = obstacle.initialPosition;

		if(obstacle.moveX)
		{
			newPosition.x += offset;
		}
		if(obstacle.moveY)
		{
			newPosition.y += offset;
		}
		if(obstacle.moveZ)
		{
			obstacle.zPosition = obstacle.baseZPosition + offset;
		}
		obstacleBody->MoveTo(newPosition);	
	}
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::cOneShotGame::Initialize()
{
	auto result = Results::Success;
	srand(static_cast<unsigned int>(time(nullptr)));

	if (IsGameEndedBefore())
	{
		const auto result = Exit(EXIT_SUCCESS);
		EAE6320_ASSERT(result);
	}
	
	//Setup the first person Camera
	{
		uint16_t width = 0;
		uint16_t height = 0;
		if (!eae6320::Application::iApplication::GetCurrentResolution(width, height))
		{
			EAE6320_ASSERTF(false, "Failed to get current resolution");
			return result;
		}
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);

		if (!(result = eae6320::Serialization::LoadCameraFromFile(firstPersonCameraFilePath.c_str(), firstPersonCamera)))
		{
			EAE6320_ASSERTF(false, "Camera failed to load");
			return result;
		}

		if(!(result = eae6320::Serialization::LoadCameraFromFile(thirdPersonCameraFilePath.c_str(), thirdPersonCamera)))
		{
			EAE6320_ASSERTF(false, "Camera failed to load");
			return result;
		}
	}

	{
		m_PhysicsWorld = std::make_unique<Physics::cPhysicsWorld>();
	}

	//Ground Object
	{
		//Load the ground effect
		if (!(result = Graphics::cEffect::Load(groundObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load ground effect");
			return result;
		}

		//Load the ground mesh
		if (!(result = Graphics::cMesh::Load(groundObject->GetMesh(), groundMayaMeshFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load ground mesh");
			return result;
		}
		
		//Load the ground texture
		if (!(result = Texture::cTexture::Load(groundTexture, groundTextureFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load ground texture");
			return result;
		}

		//Create the ground phyiscs body
		{
			m_PhysicsWorld.get()->AddBody(Physics::CreateBoxBody(15.f, 1.f, 1.f, true, 1.f));
			m_PhysicsWorld.get()->GetBody(0, groundObject->GetPhysicsBody2D());
		}
	}

	//Player Object
	{
		//Load the player effect
		if (!(result = Graphics::cEffect::Load(playerObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load player effect");
			return result;
		}
		//Load the player mesh
		if (!(result = Graphics::cMesh::Load(playerObject->GetMesh(), playerMayaMeshFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load player mesh");
			return result;
		}
		//Load the player texture
		if (!(result = Texture::cTexture::Load(playerTexture, playerTextureFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load player texture");
			return result;
		}
		//Create the player phyiscs body
		{
			m_PhysicsWorld.get()->AddBody(Physics::CreateBoxBody(1.f, 2.f, 1.f, false, 1.f));
			m_PhysicsWorld.get()->GetBody(1, playerObject->GetPhysicsBody2D());
			//playerObject->GetPhysicsBody2D()->MoveTo(Math::sVector2(0.f, 5.f));
			playerObject->GetPhysicsBody2D()->position = Math::sVector2(0.f, 0.f);
		}
	}

	//Projectile Object
	{
		//Load the projectile effect
		if (!(result = Graphics::cEffect::Load(projectileObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load projectile effect");
			return result;
		}
		//Load the projectile mesh
		if (!(result = Graphics::cMesh::Load(projectileObject->GetMesh(), projectileMayaMeshFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load projectile mesh");
			return result;
		}
		//Load the projectile texture
		if (!(result = Texture::cTexture::Load(projectileTexture, projectileTextureFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load projectile texture");
			return result;
		}
		//Create the projectile physics body
		{
			std::string errorMessage;
			m_PhysicsWorld.get()->AddBody(Physics::CreateCircleBody(0.3f, 0.3f, false, 1.f, errorMessage));
			const auto projectileIndex = m_PhysicsWorld.get()->BodyCount() - 1;
			m_PhysicsWorld.get()->GetBody(projectileIndex, projectileObject->GetPhysicsBody2D());

			//Set the projectile position to player position
			projectileObject->GetPhysicsBody2D()->position = eae6320::Math::sVector2(
				playerObject->GetPhysicsBody2D()->position.x,
				playerObject->GetPhysicsBody2D()->position.y);
		}
	}

	//Goal Object
	{
		//Load the goal effect
		if (!(result = Graphics::cEffect::Load(goalObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load goal effect");
			return result;
		}
		//Load the goal mesh
		if (!(result = Graphics::cMesh::Load(goalObject->GetMesh(), goalMayaMeshFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load goal mesh");
			return result;
		}
		//Load the goal texture
		if (!(result = Texture::cTexture::Load(goalTexture, goalTextureFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load goal texture");
			return result;
		}
		//Create the goal physics body
		{
			m_PhysicsWorld.get()->AddBody(Physics::CreateBoxBody(2.f, 2.f, 1.f, false, 1.f));
			const auto goalIndex = m_PhysicsWorld.get()->BodyCount() - 1;
			m_PhysicsWorld.get()->GetBody(goalIndex, goalObject->GetPhysicsBody2D());
			const auto& playerPos = playerObject->GetPhysicsBody2D()->position;
			goalObject->GetPhysicsBody2D()->position = eae6320::Math::sVector2(playerPos.x - 2.5f, playerPos.y);
		}
	}

	//Skybox Object
	{
		//Load the skybox effect
		if (!(result = Graphics::cEffect::Load(skyboxObject->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load skybox effect");
			return result;
		}
		//Load the skybox mesh
		if (!(result = Graphics::cMesh::Load(skyboxObject->GetMesh(), skyboxMayaMeshFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load skybox mesh");
			return result;
		}
		//Load the skybox texture
		if (!(result = Texture::cTexture::Load(skyboxTexture, skyboxTextureFilePath)))
		{
			EAE6320_ASSERTF(false, "Failed to load skybox texture");
			return result;
		}
	}

	//Create Random Obstacles
	{
		if (!(result = CreateFixedObstacles()))
		{
			EAE6320_ASSERTF(false, "Failed to create random obstacles");
			return result;
		}
	}

	//Set the camera to track the player
	{
		thirdPersonCamera.SetTrackingTarget(&playerStartPosition, &playerStartOrientation);
		//thirdPersonCamera.SetOrientation(eae6320::Math::cQuaternion::FromEulerAngles({ 0.f, 1.57f, 0.f }));
	}

	//Set the camera to track the projectile
	{

		const auto& currentProjectilePosition = projectileObject->GetPhysicsBody2D()->position;
		projectileStartPosition = eae6320::Math::sVector(currentProjectilePosition.x, currentProjectilePosition.y, 0.f);

		firstPersonCamera.SetTrackingTarget(&projectileStartPosition, &projectileStartOrientation);
	}

	return result;
}

eae6320::cResult eae6320::cOneShotGame::CreateFixedObstacles()
{
	auto result = Results::Success;

	constexpr int obstacleCount = 7;
	const int positionCount = static_cast<int>(sizeof(obstacleXYPositions) / sizeof(obstacleXYPositions[0]));

	for (int i = 0; i < obstacleCount; ++i)
	{
		eae6320::cOneShotGame::sObstacle obstacle;
		obstacle.object = std::make_shared<GameObject::cMyGameObject>(obstacleMesh, obstacleEffect);

		//Obstacle Objects
		{
			//Load the obstacle effect
			if (!(result = Graphics::cEffect::Load(obstacle.object->GetEffect(), pixelColorFilePath, vertexColorFilePath)))
			{
				EAE6320_ASSERTF(false, "Failed to load obstacle effect");
				return result;
			}
			//Load the obstacle mesh
			if (!(result = Graphics::cMesh::Load(obstacle.object->GetMesh(), obstacleMayaMeshFilePath)))
			{
				EAE6320_ASSERTF(false, "Failed to load obstacle mesh");
				return result;
			}
			//Load the obstacle texture
			if (!(result = Texture::cTexture::Load(obstacleTexture, obstacleTextureFilePath)))
			{
				EAE6320_ASSERTF(false, "Failed to load obstacle texture");
				return result;
			}
		}

		std::string errorMessage;
		m_PhysicsWorld.get()->AddBody(Physics::CreateBoxBody(1.f, 3.f, 1.f, true, 1.f));
		const auto obstacleIndex = m_PhysicsWorld.get()->BodyCount() - 1;
		m_PhysicsWorld.get()->GetBody(obstacleIndex, obstacle.object->GetPhysicsBody2D());

		const int positionIndex = i % positionCount;
		const auto& basePosition = obstacleXYPositions[positionIndex];

		obstacle.object->GetPhysicsBody2D()->position = eae6320::Math::sVector2(basePosition.x, basePosition.y);
		obstacle.zPosition = obstacleZPositions[i];

		obstacle.initialPosition = obstacle.object->GetPhysicsBody2D()->position;
		obstacle.baseZPosition = obstacle.zPosition;
		if (i % 3 == 0)
		{
			obstacle.moveZ = true;
		}
		else if( i % 3 == 1)
		{
			obstacle.moveX = true;
		}
		else
		{
			obstacle.moveY = true;
		}
		obstacle.amplitude = 1.5f + 0.3f * static_cast<float>(i % 3);
		obstacle.frequency = 0.5f + 0.1f * static_cast<float>(i % 2);

		m_Obstacles.push_back(obstacle);
	}

	return result;
}

bool eae6320::cOneShotGame::CheckProjectileObstacleCollision()
{
	auto* projectileBody = projectileObject->GetPhysicsBody2D();

	//Query circle overlap at the projectile position
	std::vector<eae6320::Physics::PhysicsBody2D*> hitBodies;

	if (!projectileBody)
	{
		return false;
	}

	const float positionY = projectileBody->position.y;
	const float positionX = projectileBody->position.x;

	const float hitDistance = projectileBody->Radius + 1.25f; //Assuming obstacle radius is 1.5f
	constexpr float hitThresholdZ = 0.5f;

	for (const auto& obstacle : m_Obstacles)
	{
		const auto& obstaclePos = obstacle.object->GetPhysicsBody2D()->position;
		//Check XYZ distance
		const float deltaX = positionX - obstaclePos.x;
		const float deltaY = positionY - obstaclePos.y;
		const float distanceXY = sqrtf(deltaX * deltaX + deltaY * deltaY);

		if (distanceXY <= hitDistance)
		{
			if (fabsf(m_ProjectileZ - obstacle.zPosition) < hitThresholdZ)
			{
				return true;
			}
		}
	}

	return false;
}

bool eae6320::cOneShotGame::CheckProjectileGoalCollision()
{
	auto* projectileBody = projectileObject->GetPhysicsBody2D();
	auto* goalBody = goalObject->GetPhysicsBody2D();

	if (!projectileBody || !goalBody)
	{
		return false;
	}

	constexpr float goalHalfWidth = 1.0f;
	constexpr float goalHalfHeight = 1.0f;

	//Query circle overlap at the projectile positionAAA
	std::vector<eae6320::Physics::PhysicsBody2D*> hitBodies;

	if(!m_PhysicsWorld.get()->OverlapBox(
		eae6320::Math::sVector2(projectileBody->position.x, projectileBody->position.y),
		1.f, 1.f, hitBodies))
	{
		return false;
	}

	constexpr float projectileLengthZ = 2.2f;
	constexpr float hitThresholdZ = 0.5f;

	const float projectileTipZ = m_ProjectileZ + projectileLengthZ;

	const float minZ = std::min(projectilePreviousZ, projectileTipZ);
	const float maxZ = std::max(projectilePreviousZ, projectileTipZ);

	if(m_goalZPosition < (minZ - hitThresholdZ) || m_goalZPosition > (maxZ + hitThresholdZ))
	{
		return false;
	}

	return true;
}

bool eae6320::cOneShotGame::IsGameEndedBefore()
{
	const auto path = Platform::GetAppDataFolderPath();
	const auto filePath = path + "/OneShotGame.lock";

	return Platform::DoesFileExist(filePath.c_str());
}

void eae6320::cOneShotGame::MarkGameAsEnded()
{
	const auto path = Platform::GetAppDataFolderPath();
	const auto filePath = path + "/OneShotGame.lock";
	std::ofstream lockFile(filePath);
	if (lockFile.is_open())
	{
		lockFile << "You only get one shot";
		lockFile.close();
	}

	m_isGameEnded = true;
}

eae6320::cResult eae6320::cOneShotGame::CleanUp()
{
	if (groundObject)
	{
		delete groundObject;
		groundObject = nullptr;
	}

	if(playerObject)
	{
		delete playerObject;
		playerObject = nullptr;
	}

	if(projectileObject)
	{
		delete projectileObject;
		projectileObject = nullptr;
	}

	if(goalObject)
	{
		delete goalObject;
		goalObject = nullptr;
	}

	if (skyboxObject)
	{
		delete skyboxObject;
		skyboxObject = nullptr;
	}

	return Results::Success;
}
