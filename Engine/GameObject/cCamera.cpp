#include "cCamera.h"

#include <Engine/Physics/sRigidBodyState.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/Functions.h>

eae6320::GameObject::cCamera::cCamera()
{
	m_rigidBodyState = new eae6320::Physics::sRigidBodyState();
	m_transform = new eae6320::Math::cMatrix_transformation();
	m_configurations = new eae6320::Math::cMatrix_transformation();
	m_rigidBodyState->position = Math::sVector( 0.f, 0.f, 10.f );
	*m_transform = Math::cMatrix_transformation::CreateWorldToCameraTransform(m_rigidBodyState->orientation, m_rigidBodyState->position);
	*m_configurations = Math::cMatrix_transformation::CreateCameraToProjectedTransform_perspective(Math::ConvertDegreesToRadians(45.f), 1.f, 0.1f, 150.f);
	m_rigidBodyState->position = m_transform->GetTranslation();
}

const eae6320::Math::cMatrix_transformation eae6320::GameObject::cCamera::GetCameraTransform()
{
	return *m_transform;
}

const eae6320::Math::sVector eae6320::GameObject::cCamera::GetCameraPosition() const
{
	return m_rigidBodyState->position;
}

const eae6320::Math::cQuaternion eae6320::GameObject::cCamera::GetCameraRotation() const
{
	return m_rigidBodyState->orientation;
}

const eae6320::Math::cMatrix_transformation eae6320::GameObject::cCamera::GetCameraConfigurations()
{
	return *m_configurations;
}

eae6320::Physics::sRigidBodyState*& eae6320::GameObject::cCamera::GetRigidBodyState()
{
	return m_rigidBodyState;
}

eae6320::GameObject::cCamera::~cCamera()
{
	if (m_rigidBodyState)
	{
		delete m_rigidBodyState;
	}

	if (m_transform)
	{
		delete m_transform;
	}

	if (m_configurations)
	{
		delete m_configurations;
	}
}
