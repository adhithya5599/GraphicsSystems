#pragma once
/*
	This class if for Gameobjects
*/

#ifndef EAE6320_GAMEOBJECT_CGAMEOBJECT_H
#define EAE6320_GAMEOBJECT_CGAMEOBJECT_H

#include <Engine/Results/Results.h>

namespace eae6320
{
	namespace Graphics
	{
		class cMesh;
		class cEffect;
	}

	namespace Physics
	{
		class PhysicsBody2D;
	}
}

namespace eae6320
{
	namespace GameObject
	{
		class cMyGameObject
		{
		public:
			cMyGameObject(Graphics::cMesh*& o_mesh, Graphics::cEffect*& o_effect);
			~cMyGameObject();

			inline Graphics::cMesh*& GetMesh() { return m_Mesh; };
			inline Graphics::cEffect*& GetEffect() { return m_Effect; };

			//Physics::sRigidBodyState* GetRigidBodyState();
			inline Physics::PhysicsBody2D*& GetPhysicsBody2D() { return m_PhysicsBody2D; };

		private:
			// These must start as nullptr:
			// the destructor releases them if they aren't null,
			// so an uninitialized (garbage) pointer would be "released" if a mesh or effect was never loaded
			Graphics::cMesh* m_Mesh = nullptr;
			Graphics::cEffect* m_Effect = nullptr;

			Physics::sRigidBodyState* m_RigidBodyState = nullptr;
			//Physics::sRigidBodyState* m_RigidBodyState;
			Physics::PhysicsBody2D* m_PhysicsBody2D;
		};
	}
}

#endif