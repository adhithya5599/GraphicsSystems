#pragma once

#include <Engine/Math/sVector2.h>
#include <vector>

namespace eae6320
{
	namespace Physics
	{
		class PhysicsBody2D;
		struct AABB;

		bool IntersectAABBs(const AABB& a, const AABB& b);

		void FindContactPoints(
			PhysicsBody2D* i_bodyA, PhysicsBody2D* i_bodyB,
			Math::sVector2& o_contact1, Math::sVector2& o_contact2, int& o_contactCount);

		static void FindCirclePolygonContactPoint(const Math::sVector2& i_circleCenter, float i_circleRadius,
			const Math::sVector2& i_polygonCenter, const std::vector<Math::sVector2>& i_polygonVertices,
			Math::sVector2& o_contactPoint);

		static void FindPolygonsContactPoints(const std::vector<Math::sVector2>& i_verticesA, const std::vector<Math::sVector2>& i_verticesB,
			Math::sVector2& o_contact1, Math::sVector2& o_contact2, int& o_contactCount);

		static void FindCircleCircleContactPoint(const Math::sVector2& i_centerA, float i_radiusA, const Math::sVector2& i_centerB, Math::sVector2& o_cp);

		bool Collide(PhysicsBody2D* bodyA, PhysicsBody2D* bodyB, Math::sVector2& o_normal, float& o_depth);

		bool Collide(PhysicsBody2D* bodyA, PhysicsBody2D* bodyB);

		bool IntersectCircles(const Math::sVector2& i_centerA, float i_radiusA, const Math::sVector2& i_centerB, float i_radiusB,
			Math::sVector2& o_normal, float& o_depth);

		bool IntersectPolygons(const Math::sVector2& i_polygonCenterA, const std::vector<Math::sVector2>& i_verticesA,
			const Math::sVector2& i_polygonCenterB, const std::vector<Math::sVector2>& i_verticesB,
			Math::sVector2& o_normal, float& o_depth);

		bool IntersectCirclePolygon(const Math::sVector2& i_circleCenter, const float& i_circleRadius, const Math::sVector2& i_polygonCenter, const std::vector<Math::sVector2>& i_vertices,
			Math::sVector2& o_normal, float& o_depth);

		void PointSegmentDistance(const Math::sVector2& i_p, const Math::sVector2& i_a, const Math::sVector2& i_b,
			float& o_distSqr, Math::sVector2& o_cp);

		// Raycast result structure
		struct RaycastHit
		{
			PhysicsBody2D* Body;          // The body that was hit
			Math::sVector2 Point;         // The hit point in world space
			Math::sVector2 Normal;        // The surface normal at hit point
			float Distance;               // Distance from ray origin to hit point

			RaycastHit()
				: Body(nullptr), Point(Math::sVector2::Zero()), Normal(Math::sVector2::Zero()), Distance(FLT_MAX) {}
		};

		// Raycast helper functions
		bool RayIntersectAABB(const Math::sVector2& i_rayOrigin, const Math::sVector2& i_rayDir,
			float i_rayDistance, const AABB& i_aabb);

		bool RayIntersectCircle(const Math::sVector2& i_rayOrigin, const Math::sVector2& i_rayDir,
			float i_rayDistance, const Math::sVector2& i_circleCenter, float i_radius,
			Math::sVector2& o_hitPoint, Math::sVector2& o_normal, float& o_distance);

		bool RayIntersectBox(const Math::sVector2& i_rayOrigin, const Math::sVector2& i_rayDir,
			float i_rayDistance, const std::vector<Math::sVector2>& i_vertices,
			Math::sVector2& o_hitPoint, Math::sVector2& o_normal, float& o_distance);
	}
}