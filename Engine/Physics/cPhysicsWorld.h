#pragma once

#include <vector>
#include <Engine/Math/sVector2.h>
#include <Engine/Physics/cPhysicsBody2D.h>
#include "PhysicsManifold.h"
#include <memory>
#include <map>

namespace eae6320
{
	namespace Physics
	{
		class cPhysicsWorld
		{
		public:
			cPhysicsWorld()
			{
				gravity = Math::sVector2(0.f, -9.81f);
				contactList.resize(2);
				impulseList.resize(2);
				raList.resize(2);
				rbList.resize(2);
				frictionImpulseList.resize(2);
				jList.resize(2);
				m_contactPairs.resize(0);
			}

			void AddBody(std::unique_ptr<PhysicsBody2D> i_body);
			void RemoveBody(PhysicsBody2D* i_body);
			bool GetBody(int i_index, PhysicsBody2D*& o_body);
			// 改变iterations来增加运行的精度，代表了在一次step中，进行多少次substeps
			void Step(float i_deltatime, int i_iterations);
			void BroadPhase();
			void NarrowPhase();
			void StepBodies(float time, int totalIterations);
			void ResolveCollisionBasic(const PhysicsManifold& i_contact);
			void ResolveCollisionWithRotation(const PhysicsManifold& i_contact);
			void ResolveCollisionWithRotationAndFriction(const PhysicsManifold& i_contact);
			void SeparateBodies(PhysicsBody2D* i_bodyA, PhysicsBody2D* i_bodyB, Math::sVector2 i_mtv);

			// Collision Test
			bool OverlapBox(const Math::sVector2& i_boxPosition, float i_boxWidth, float i_boxHeight, std::vector<PhysicsBody2D*>& o_result);
			bool OverlapCircle(const Math::sVector2& i_circlePosition, float i_radius, std::vector<PhysicsBody2D*>& o_result);
			bool RayCast(const Math::sVector2& i_rayStartPos, const Math::sVector2& i_rayDir, float i_rayDistance,
				std::vector<PhysicsBody2D*>& o_result, std::vector<Math::sVector2>& o_hitPoints);

			int BodyCount() const { return (int)m_bodyList.size(); }
		public:
			static constexpr float MinBodySize = 0.01f * 0.01f;
			static constexpr float MaxBodySize = 64.f * 64.f;

			static constexpr float MinDensity = 0.2f;
			static constexpr float MaxDensity = 21.4f;

			static constexpr int MinIterations = 1;
			static constexpr int MaxInterations = 128;

			std::vector<Math::sVector2> ContactPointList;

		private:
			std::vector<eae6320::Math::sVector2> contactList;
			std::vector<eae6320::Math::sVector2> impulseList;
			std::vector<eae6320::Math::sVector2> frictionImpulseList;
			std::vector<eae6320::Math::sVector2> raList;
			std::vector<eae6320::Math::sVector2> rbList;
			std::vector<float>					 jList;

			//Physics bodies
			std::vector<std::unique_ptr<PhysicsBody2D>> m_bodyList;
			Math::sVector2 gravity;
			std::vector<std::pair<int, int>> m_contactPairs;
		};
	}
}