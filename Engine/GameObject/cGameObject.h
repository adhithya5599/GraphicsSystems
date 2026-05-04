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
		struct sRigidBodyState;
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

			Graphics::cMesh*& GetMesh();
			Graphics::cEffect*& GetEffect();

			Physics::sRigidBodyState* GetRigidBodyState();

		private:
			Graphics::cMesh* m_Mesh;
			Graphics::cEffect* m_Effect;

			Physics::sRigidBodyState* m_RigidBodyState;
		};
	}
}

#endif