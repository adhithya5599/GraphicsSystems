#include "cGameObject.h"

#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/cEffect.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/ScopeGuard/cScopeGuard.h>

eae6320::GameObject::cMyGameObject::cMyGameObject(Graphics::cMesh*& o_mesh, Graphics::cEffect*& o_effect)
{
	auto result = Results::Success;
	cScopeGuard scopeGuard([&o_effect, &result, this, &o_mesh]
		{
			if (result)
			{
				EAE6320_ASSERT(m_Effect != nullptr);
				o_effect = m_Effect;

				EAE6320_ASSERT(m_Mesh != nullptr);
				o_mesh = m_Mesh;
			}
			else
			{
				if (m_Effect)
				{
					m_Effect->DecrementReferenceCount();
					m_Effect = nullptr;
				}
				o_effect = nullptr;

				if (m_Mesh)
				{
					m_Mesh->DecrementReferenceCount();
					m_Mesh = nullptr;
				}
				o_mesh = nullptr;
			}
		});
	//m_RigidBodyState = new Physics::sRigidBodyState();
	//m_RigidBodyState->position = Math::sVector(0.0f, 0.0f, 0.0f);
}

eae6320::GameObject::cMyGameObject::~cMyGameObject()
{
	auto result = Results::Success;
	//if (m_RigidBodyState)
	//{
	//	delete m_RigidBodyState;
	//}

	if (m_Mesh)
	{
		m_Mesh->DecrementReferenceCount();
		m_Mesh = nullptr;
	}

	if (m_Effect)
	{
		m_Effect->DecrementReferenceCount();
		m_Effect = nullptr;
	}
	EAE6320_ASSERT(result);
}
