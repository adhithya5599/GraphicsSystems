#pragma once
#include "cPhysicsBody2D.h"
#include <Engine/Math/sVector2.h>

namespace eae6320
{
	namespace Physics
	{
		// 用于在判断完Collide阶段后，存储信息，供后面ResolveCollision阶段使用
		struct PhysicsManifold
		{
			PhysicsBody2D* BodyA;
			PhysicsBody2D* BodyB;
			const Math::sVector2 Normal;
			const float Depth;
			const Math::sVector2 Contact1;
			const Math::sVector2 Contact2;
			const int ContactCount;

			PhysicsManifold(PhysicsBody2D* i_bodyA, PhysicsBody2D* i_bodyB, const Math::sVector2& i_normal, float i_depth,
				const Math::sVector2& i_contact1, const Math::sVector2& i_contact2, int i_contactCount)
				:BodyA(i_bodyA), BodyB(i_bodyB), Normal(i_normal), Depth(i_depth), Contact1(i_contact1), Contact2(i_contact2),
				ContactCount(i_contactCount)
			{

			}
		};
	}
}