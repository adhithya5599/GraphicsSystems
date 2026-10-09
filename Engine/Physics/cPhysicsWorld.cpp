#include "cPhysicsWorld.h"
#include <Engine/Physics/Collisions.h>
#include <Engine/Physics/PhysicsUtil.h>
#include <Engine/Math/Functions.h>
#include <algorithm>

using namespace eae6320::Physics;

void eae6320::Physics::cPhysicsWorld::AddBody(std::unique_ptr<PhysicsBody2D> i_body)
{
	m_bodyList.emplace_back(std::move(i_body));
}

void eae6320::Physics::cPhysicsWorld::RemoveBody(PhysicsBody2D* i_body)
{
	for (auto iter = m_bodyList.begin(); iter != m_bodyList.end(); ++iter)
	{
		if ((*iter).get() == i_body)
		{
			m_bodyList.erase(iter);
			break;
		}
	}
}

bool eae6320::Physics::cPhysicsWorld::GetBody(int i_index, PhysicsBody2D*& o_body)
{
	o_body = nullptr;
	if (i_index < 0 || i_index >= static_cast<int>(m_bodyList.size()))
	{
		return false;
	}

	o_body = m_bodyList[i_index].get();
	return true;
}

void eae6320::Physics::cPhysicsWorld::Step(float i_deltatime, int i_iterations)
{
	i_iterations = Util::Clamp(i_iterations, MinIterations, MaxInterations);

	ContactPointList.clear();

	for (int currentIter = 0; currentIter < i_iterations; currentIter++)
	{
		m_contactPairs.clear();
		StepBodies(i_deltatime, i_iterations);
		BroadPhase();
		NarrowPhase();
	}
}

void eae6320::Physics::cPhysicsWorld::BroadPhase()
{
	// Collision Step
	// (size() is unsigned, so "size() - 1" would wrap around to a huge number when there are no bodies;
	// "i + 1 < size()" checks the same thing without subtracting)
	for (unsigned int i = 0; i + 1 < m_bodyList.size(); i++)
	{
		Physics::PhysicsBody2D* bodyA = m_bodyList[i].get();
		Physics::AABB bodyA_AABB = bodyA->GetAABB();

		for (unsigned int j = i + 1; j < m_bodyList.size(); j++)
		{
			Physics::PhysicsBody2D* bodyB = m_bodyList[j].get();
			Physics::AABB bodyB_AABB = bodyB->GetAABB();

			if (bodyA->bIsStatic && bodyB->bIsStatic)
			{
				continue;
			}

			// The AABB test is a cheap, rough check that skips the expensive exact test for most pairs
			if (!IntersectAABBs(bodyA_AABB, bodyB_AABB))
			{
				continue;
			}

			m_contactPairs.emplace_back(std::pair<int, int>(i, j));
		}
	}
}

void eae6320::Physics::cPhysicsWorld::NarrowPhase()
{
	for (unsigned int i = 0; i < m_contactPairs.size(); i++)
	{
		std::pair<int, int> pair = m_contactPairs[i];
		PhysicsBody2D* bodyA = m_bodyList[pair.first].get();
		PhysicsBody2D* bodyB = m_bodyList[pair.second].get();

		Math::sVector2 normal;
		float depth;

		if (Collide(bodyA, bodyB, normal, depth))
		{
			SeparateBodies(bodyA, bodyB, normal * depth);

			Math::sVector2 contact1, contact2;
			int contactCount;

			FindContactPoints(bodyA, bodyB, contact1, contact2, contactCount);

			PhysicsManifold contact = PhysicsManifold(bodyA, bodyB, normal, depth, contact1, contact2, contactCount);
			ResolveCollisionWithRotationAndFriction(contact);
		}

		//ResolveCollisionWithRotationAndFriction(m_contactList[i]);

		//ContactPointList.emplace_back(m_contactList[i].Contact1);

		//if (m_contactList[i].ContactCount > 1)
		//{
		//	ContactPointList.emplace_back(m_contactList[i].Contact2);
		//}
	}
}

void eae6320::Physics::cPhysicsWorld::StepBodies(float time, int totalIterations)
{
	// Movement Step
	for (unsigned int i = 0; i < m_bodyList.size(); i++)
	{
		m_bodyList[i]->Step(time, gravity, totalIterations);
	}
}

// Uses the impulse equation to calculate how the two bodies' velocities change because of the collision
void eae6320::Physics::cPhysicsWorld::ResolveCollisionBasic(const PhysicsManifold& i_manifold)
{
	Math::sVector2 relativeVelocity = i_manifold.BodyB->velocity - i_manifold.BodyA->velocity;

	// If the bodies are already moving apart along the normal then there is nothing to bounce
	if (Math::Dot(relativeVelocity, i_manifold.Normal) > 0.f)
	{
		return;
	}

	float e = std::min(i_manifold.BodyA->Restitution, i_manifold.BodyB->Restitution);
	// The size of the bounce depends on the restitution ("bounciness")
	float j = -(1.f + e) * Math::Dot(relativeVelocity, i_manifold.Normal);
	j /= (i_manifold.BodyA->InvMass) + (i_manifold.BodyB->InvMass);

	Math::sVector2 impulse = j * i_manifold.Normal;

	i_manifold.BodyA->velocity -= impulse * i_manifold.BodyA->InvMass;
	i_manifold.BodyB->velocity += impulse * i_manifold.BodyB->InvMass;
}



void eae6320::Physics::cPhysicsWorld::ResolveCollisionWithRotation(const PhysicsManifold& i_contact)
{
	PhysicsBody2D* bodyA = i_contact.BodyA;
	PhysicsBody2D* bodyB = i_contact.BodyB;
	Math::sVector2 normal = i_contact.Normal;
	Math::sVector2 contact1 = i_contact.Contact1;
	Math::sVector2 contact2 = i_contact.Contact2;
	int contactCount = i_contact.ContactCount;

	float e = std::min(bodyA->Restitution, bodyB->Restitution);
	contactList[0] = contact1;
	contactList[1] = contact2;

	for (int i = 0; i < contactCount; i++)
	{
		impulseList[i] = Math::sVector2::Zero();
		raList[i] = Math::sVector2::Zero();
		rbList[i] = Math::sVector2::Zero();
	}

	for (int i = 0; i < contactCount; i++)
	{
		Math::sVector2 ra = contactList[i] - bodyA->position;
		Math::sVector2 rb = contactList[i] - bodyB->position;
		raList[i] = ra;
		rbList[i] = rb;

		Math::sVector2 raPerp = Math::sVector2(-ra.y, ra.x);
		Math::sVector2 rbPerp = Math::sVector2(-rb.y, rb.x);

		Math::sVector2 angularLinearVelocityA = raPerp * bodyA->angularVelocity;
		Math::sVector2 angularLinearVelocityB = rbPerp * bodyB->angularVelocity;


		Math::sVector2 relativeVelocity =
			(i_contact.BodyB->velocity + angularLinearVelocityB) -
			(i_contact.BodyA->velocity - angularLinearVelocityA);

		float contactVelocityMag = Math::Dot(relativeVelocity, i_contact.Normal);

		if (contactVelocityMag > 0.f)
		{
			continue;
		}

		float raPerpDotN = Math::Dot(raPerp, normal);
		float rbPerpDotN = Math::Dot(rbPerp, normal);

		float denom = (i_contact.BodyA->InvMass) + (i_contact.BodyB->InvMass) +
			raPerpDotN * raPerpDotN * bodyA->InvInertia +
			rbPerpDotN * rbPerpDotN * bodyB->InvInertia;

		float j = -(1.f + e) * contactVelocityMag;
		j /= denom;
		j /= (float)contactCount;

		Math::sVector2 impulse = j * normal;
		impulseList[i] = impulse;
	}

	for (int i = 0; i < contactCount; i++)
	{
		Math::sVector2 impulse = impulseList[i];

		bodyA->velocity += -impulse * bodyA->InvMass;
		// The cross product measures how much the impulse changes the angular velocity: an impulse straight along r
		// (from the center of mass to the contact point) doesn't rotate the body, and the more perpendicular they are the more it does
		bodyA->angularVelocity += -Math::Cross(raList[i], impulse) * bodyA->InvInertia;
		bodyB->velocity += impulse * bodyB->InvMass;
		bodyB->angularVelocity += Math::Cross(rbList[i], impulse) * bodyB->InvInertia;
	}
}

void eae6320::Physics::cPhysicsWorld::ResolveCollisionWithRotationAndFriction(const PhysicsManifold& i_contact)
{
	PhysicsBody2D* bodyA = i_contact.BodyA;
	PhysicsBody2D* bodyB = i_contact.BodyB;
	Math::sVector2 normal = i_contact.Normal;
	Math::sVector2 contact1 = i_contact.Contact1;
	Math::sVector2 contact2 = i_contact.Contact2;
	int contactCount = i_contact.ContactCount;

	float e = std::min(bodyA->Restitution, bodyB->Restitution);

	// A table of per-material-pair friction values could be used here; the average of the two bodies is used instead
	float sf = (bodyA->StaticFriction + bodyB->StaticFriction) / 2.f;
	float df = (bodyA->DynamicFriction + bodyB->DynamicFriction) / 2.f;

	contactList[0] = contact1;
	contactList[1] = contact2;

	for (int i = 0; i < contactCount; i++)
	{
		impulseList[i] = Math::sVector2::Zero();
		frictionImpulseList[i] = Math::sVector2::Zero();
		raList[i] = Math::sVector2::Zero();
		rbList[i] = Math::sVector2::Zero();
		jList[i] = 0.f;
	}

	for (int i = 0; i < contactCount; i++)
	{
		Math::sVector2 ra = contactList[i] - bodyA->position;
		Math::sVector2 rb = contactList[i] - bodyB->position;
		raList[i] = ra;
		rbList[i] = rb;

		Math::sVector2 raPerp = Math::sVector2(-ra.y, ra.x);
		Math::sVector2 rbPerp = Math::sVector2(-rb.y, rb.x);

		Math::sVector2 angularLinearVelocityA = raPerp * bodyA->angularVelocity;
		Math::sVector2 angularLinearVelocityB = rbPerp * bodyB->angularVelocity;

		Math::sVector2 relativeVelocity =
			(i_contact.BodyB->velocity + angularLinearVelocityB) -
			(i_contact.BodyA->velocity + angularLinearVelocityA);

		float contactVelocityMag = Math::Dot(relativeVelocity, i_contact.Normal);

		if (contactVelocityMag > 0.f)
		{
			continue;
		}

		float raPerpDotN = Math::Dot(raPerp, normal);
		float rbPerpDotN = Math::Dot(rbPerp, normal);

		float denom = (i_contact.BodyA->InvMass) + (i_contact.BodyB->InvMass) +
			raPerpDotN * raPerpDotN * bodyA->InvInertia +
			rbPerpDotN * rbPerpDotN * bodyB->InvInertia;

		float j = -(1.f + e) * contactVelocityMag;
		j /= denom;
		j /= (float)contactCount;
		jList[i] = j;

		Math::sVector2 impulse = j * normal;
		impulseList[i] = impulse;
	}

	for (int i = 0; i < contactCount; i++)
	{
		Math::sVector2 impulse = impulseList[i];

		bodyA->velocity += -impulse * bodyA->InvMass;
		// The cross product measures how much the impulse changes the angular velocity: an impulse straight along r
		// (from the center of mass to the contact point) doesn't rotate the body, and the more perpendicular they are the more it does
		bodyA->angularVelocity += -Math::Cross(raList[i], impulse) * bodyA->InvInertia * 0.5f;
		bodyB->velocity += impulse * bodyB->InvMass;
		bodyB->angularVelocity += Math::Cross(rbList[i], impulse) * bodyB->InvInertia * 0.5f;
	}

	for (int i = 0; i < contactCount; i++)
	{
		Math::sVector2 ra = contactList[i] - bodyA->position;
		Math::sVector2 rb = contactList[i] - bodyB->position;
		raList[i] = ra;
		rbList[i] = rb;

		Math::sVector2 raPerp = Math::sVector2(-ra.y, ra.x);
		Math::sVector2 rbPerp = Math::sVector2(-rb.y, rb.x);

		Math::sVector2 angularLinearVelocityA = raPerp * bodyA->angularVelocity;
		Math::sVector2 angularLinearVelocityB = rbPerp * bodyB->angularVelocity;


		Math::sVector2 relativeVelocity =
			(i_contact.BodyB->velocity + angularLinearVelocityB) -
			(i_contact.BodyA->velocity + angularLinearVelocityA);

		Math::sVector2 tangent = relativeVelocity - Math::Dot(relativeVelocity, normal) * normal;
		if (Math::NearlyEqual(tangent, Math::sVector2::Zero()))
		{
			continue;
		}
		else
		{
			tangent = tangent.GetNormalized();
		}

		float raPerpDotT = Math::Dot(raPerp, tangent);
		float rbPerpDotT = Math::Dot(rbPerp, tangent);

		float denom = (i_contact.BodyA->InvMass) + (i_contact.BodyB->InvMass) +
			raPerpDotT * raPerpDotT * bodyA->InvInertia +
			rbPerpDotT * rbPerpDotT * bodyB->InvInertia;

		float contactVelocityMag = Math::Dot(relativeVelocity, tangent);
		float jt = -contactVelocityMag;
		jt /= denom;
		jt /= (float)contactCount;

		Math::sVector2 frictionImpulse = Math::sVector2::Zero();

		if (std::abs(jt) <= jList[i] * sf)
		{
			frictionImpulse = jt * tangent;
		}
		else
		{
			float magnitude = std::abs(jList[i]) * df;
			float direction = (jt > 0.f) ? 1.f : -1.f;
			frictionImpulse = direction * magnitude * tangent;
		}
		frictionImpulseList[i] = frictionImpulse;
	}

	for (int i = 0; i < contactCount; i++)
	{
		Math::sVector2 impulse = frictionImpulseList[i];

		bodyA->velocity += -impulse * bodyA->InvMass;
		bodyA->angularVelocity += -Math::Cross(raList[i], impulse) * bodyA->InvInertia;
		bodyB->velocity += impulse * bodyB->InvMass;
		bodyB->angularVelocity += Math::Cross(rbList[i], impulse) * bodyB->InvInertia;
	}
}

void eae6320::Physics::cPhysicsWorld::SeparateBodies(PhysicsBody2D* i_bodyA, PhysicsBody2D* i_bodyB, Math::sVector2 i_mtv)
{
	// A static body can't be moved, so when one of the bodies is static the other one must move the whole distance
	// (moving it only half way would leave the bodies still overlapping, and dynamic bodies would sink into static ones)
	if (i_mtv.GetLength() <= 0.001f)
	{
		return;
	}
	if (i_bodyA->bIsStatic)
	{
		i_bodyB->Move(i_mtv);
	}
	else if (i_bodyB->bIsStatic)
	{
		i_bodyA->Move(-i_mtv);
	}
	else
	{
		i_bodyA->Move(-i_mtv / 2.f);
		i_bodyB->Move(i_mtv / 2.f);
	}
}

bool eae6320::Physics::cPhysicsWorld::OverlapBox(const Math::sVector2& i_boxPosition, float i_boxWidth, float i_boxHeight, std::vector<PhysicsBody2D*>& o_result)
{
	bool result = false;

	auto tempBody = CreateBoxBody(i_boxWidth, i_boxHeight, 1, false, 1);
	tempBody.get()->MoveTo(i_boxPosition);
	auto tempAABB = tempBody.get()->GetAABB();

	for (unsigned int j = 0; j < m_bodyList.size(); j++)
	{
		Physics::PhysicsBody2D* bodyB = m_bodyList[j].get();
		Physics::AABB bodyB_AABB = bodyB->GetAABB();

		if (!IntersectAABBs(tempAABB, bodyB_AABB))
		{
			continue;
		}

		if (Collide(tempBody.get(), bodyB))
		{
			o_result.emplace_back(bodyB);
			result = true;
		}
	}
	return result;
}

bool eae6320::Physics::cPhysicsWorld::OverlapCircle(const Math::sVector2& i_circlePosition, float i_radius, std::vector<PhysicsBody2D*>& o_result)
{
	bool result = false;

	auto tempBody = CreateCircleBody(i_radius, 1, false, 1);
	tempBody.get()->MoveTo(i_circlePosition);
	auto tempAABB = tempBody.get()->GetAABB();

	for (unsigned int j = 0; j < m_bodyList.size(); j++)
	{
		Physics::PhysicsBody2D* bodyB = m_bodyList[j].get();
		Physics::AABB bodyB_AABB = bodyB->GetAABB();

		if (!IntersectAABBs(tempAABB, bodyB_AABB))
		{
			continue;
		}

		if (Collide(tempBody.get(), bodyB))
		{
			o_result.emplace_back(bodyB);
			result = true;
		}
	}
	return result;
}

bool eae6320::Physics::cPhysicsWorld::RayCast(const Math::sVector2& i_rayStartPos, const Math::sVector2& i_rayDir,
	float i_rayDistance, std::vector<PhysicsBody2D*>& o_result, std::vector<Math::sVector2>& o_hitPoints)
{
	o_result.clear();
	o_hitPoints.clear();

	// Normalize ray direction
	Math::sVector2 rayDir = i_rayDir.GetNormalized();

	// Store all potential hits with their distances
	std::vector<RaycastHit> hits;

	// Broad phase: Check AABB intersections first
	for (unsigned int i = 0; i < m_bodyList.size(); i++)
	{
		PhysicsBody2D* body = m_bodyList[i].get();
		AABB bodyAABB = body->GetAABB();

		// Quick AABB test
		if (!RayIntersectAABB(i_rayStartPos, rayDir, i_rayDistance, bodyAABB))
		{
			continue;
		}

		// Narrow phase: Precise intersection test based on shape type
		RaycastHit hit;
		hit.Body = body;
		bool hasHit = false;

		if (body->ShapeType == EShapeType::Circle)
		{
			// Test ray against circle
			hasHit = RayIntersectCircle(
				i_rayStartPos, rayDir, i_rayDistance,
				body->position, body->Radius,
				hit.Point, hit.Normal, hit.Distance);
		}
		else if (body->ShapeType == EShapeType::Box)
		{
			// Test ray against polygon (box)
			const std::vector<Math::sVector2>& vertices = body->GetTransformedVertices2D();
			hasHit = RayIntersectBox(
				i_rayStartPos, rayDir, i_rayDistance,
				vertices,
				hit.Point, hit.Normal, hit.Distance);
		}

		if (hasHit)
		{
			hits.push_back(hit);
		}
	}

	// Sort hits by distance (nearest first)
	std::sort(hits.begin(), hits.end(),
		[](const RaycastHit& a, const RaycastHit& b) {
			return a.Distance < b.Distance;
		});

	// Copy sorted bodies and hit points to output
	for (const auto& hit : hits)
	{
		o_result.push_back(hit.Body);
		o_hitPoints.push_back(hit.Point);
	}

	return !o_result.empty();
}