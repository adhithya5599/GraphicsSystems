#include "Collisions.h"
#include "cPhysicsBody2D.h"
#include <math.h>
#include <Engine/Math/Functions.h>
#include <Engine/Physics/AABB.h>
#include <Engine/Physics/PhysicsUtil.h>

static void ProjectVertices(const std::vector<eae6320::Math::sVector2>& i_vertices, const eae6320::Math::sVector2& i_axis, float& o_min, float& o_max);
static void ProjectCircle(const eae6320::Math::sVector2& i_center, const float& i_radius, eae6320::Math::sVector2& i_axis, float& o_min, float& o_max);
static int FindClosestPointOnPolygon(const eae6320::Math::sVector2& i_circleCenter, const std::vector< eae6320::Math::sVector2>& i_vertices);

bool eae6320::Physics::IntersectAABBs(const eae6320::Physics::AABB& a, const eae6320::Physics::AABB& b)
{
    if (a.Max.x <= b.Min.x || b.Max.x <= a.Min.x ||
        a.Max.y <= b.Min.y || b.Max.y <= a.Min.y)
    {
        return false;
    }
    return true;
}

void eae6320::Physics::FindContactPoints(PhysicsBody2D* i_bodyA, PhysicsBody2D* i_bodyB, Math::sVector2& o_contact1, Math::sVector2& o_contact2, int& o_contactCount)
{
    o_contact1 = Math::sVector2::Zero();
    o_contact2 = Math::sVector2::Zero();
    o_contactCount = 0;

    EShapeType shapeTypeA = i_bodyA->ShapeType;
    EShapeType shapeTypeB = i_bodyB->ShapeType;

    if (shapeTypeA == EShapeType::Box)
    {
        if (shapeTypeB == EShapeType::Box)
        {
            FindPolygonsContactPoints(i_bodyA->GetTransformedVertices2D(), i_bodyB->GetTransformedVertices2D(), o_contact1, o_contact2, o_contactCount);
        }
        else if (shapeTypeB == EShapeType::Circle)
        {
            FindCirclePolygonContactPoint(i_bodyB->position, i_bodyB->Radius, i_bodyA->position, i_bodyA->GetTransformedVertices2D(), o_contact1);
            o_contactCount = 1;
        }
    }
    else if (shapeTypeA == EShapeType::Circle)
    {
        if (shapeTypeB == EShapeType::Box)
        {
            FindCirclePolygonContactPoint(i_bodyA->position, i_bodyA->Radius, i_bodyB->position, i_bodyB->GetTransformedVertices2D(), o_contact1);
            o_contactCount = 1;
        }
        else if (shapeTypeB == EShapeType::Circle)
        {
            FindCircleCircleContactPoint(i_bodyA->position, i_bodyA->Radius, i_bodyB->position, o_contact1);
            o_contactCount = 1;
        }
    }
}

void eae6320::Physics::FindCirclePolygonContactPoint(const Math::sVector2& i_circleCenter, float i_circleRadius,
    const Math::sVector2& i_polygonCenter, const std::vector<Math::sVector2>& i_polygonVertices,
    Math::sVector2& o_contactPoint)
{
    o_contactPoint = Math::sVector2::Zero();
    float minDistSqr = FLT_MAX;
    for (unsigned int i = 0; i < i_polygonVertices.size(); i++)
    {
        Math::sVector2 va = i_polygonVertices[i];
        Math::sVector2 vb = i_polygonVertices[(i + 1) % i_polygonVertices.size()];

        float distSqr;
        Math::sVector2 contactPoint;
        PointSegmentDistance(i_circleCenter, va, vb, distSqr, contactPoint);
        if (distSqr < minDistSqr)
        {
            minDistSqr = distSqr;
            o_contactPoint = contactPoint;
        }
    }
}

void eae6320::Physics::FindPolygonsContactPoints(const std::vector<Math::sVector2>& i_verticesA, const std::vector<Math::sVector2>& i_verticesB,
    Math::sVector2& o_contact1, Math::sVector2& o_contact2, int& o_contactCount)
{
    o_contact1 = Math::sVector2::Zero();
    o_contact2 = Math::sVector2::Zero();
    o_contactCount = 0;

    float minDistSqr = FLT_MAX;

    for (unsigned int i = 0; i < i_verticesA.size(); i++)
    {
        Math::sVector2 p = i_verticesA[i];
        for (unsigned int j = 0; j < i_verticesB.size(); j++)
        {
            Math::sVector2 va = i_verticesB[j];
            Math::sVector2 vb = i_verticesB[(j + 1) % i_verticesB.size()];

            float distSqr;
            Math::sVector2 cp;
            PointSegmentDistance(p, va, vb, distSqr, cp);

            // If this point is (almost) as close to the edge as the closest contact found so far,
            // the two bodies' colliding edges are (almost) parallel, and so this counts as a second contact point
            if (Math::NearlyEqual(distSqr, minDistSqr))
            {
                // When the two bodies' edges share (almost) the same endpoint,
                // that endpoint would be counted twice and could overwrite the other, real, contact point,
                // so a point that matches the first contact is ignored
                if (!Math::NearlyEqual(cp, o_contact1))
                {
                    o_contact2 = cp;
                    o_contactCount = 2;
                }

            }
            else if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
                o_contactCount = 1;
                o_contact1 = cp;
            }
        }
    }

    for (unsigned int i = 0; i < i_verticesB.size(); i++)
    {
        Math::sVector2 p = i_verticesB[i];
        for (unsigned int j = 0; j < i_verticesA.size(); j++)
        {
            Math::sVector2 va = i_verticesA[j];
            Math::sVector2 vb = i_verticesA[(j + 1) % i_verticesA.size()];

            float distSqr;
            Math::sVector2 cp;
            PointSegmentDistance(p, va, vb, distSqr, cp);

            if (Math::NearlyEqual(distSqr, minDistSqr))
            {
                if (!Math::NearlyEqual(cp, o_contact1))
                {
                    o_contact2 = cp;
                    o_contactCount = 2;
                }

            }
            else if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
                o_contactCount = 1;
                o_contact1 = cp;
            }
        }
    }
}

void eae6320::Physics::FindCircleCircleContactPoint(const Math::sVector2& i_centerA, float i_radiusA, const Math::sVector2& i_centerB, Math::sVector2& o_cp)
{
    Math::sVector2 ab = i_centerB - i_centerA;
    Math::sVector2 dir = ab.GetNormalized();
    o_cp = i_centerA + dir * i_radiusA;
}

bool eae6320::Physics::Collide(eae6320::Physics::PhysicsBody2D* bodyA, PhysicsBody2D* bodyB, Math::sVector2& o_normal, float& o_depth)
{
    o_normal = Math::sVector2::Zero();
    o_depth = 0;

    EShapeType shapeTypeA = bodyA->ShapeType;
    EShapeType shapeTypeB = bodyB->ShapeType;

    if (shapeTypeA == EShapeType::Box)
    {
        if (shapeTypeB == EShapeType::Box)
        {
            return Physics::IntersectPolygons(
                bodyA->position, bodyA->GetTransformedVertices2D(),
                bodyB->position, bodyB->GetTransformedVertices2D(),
                o_normal, o_depth);
        }
        else if (shapeTypeB == EShapeType::Circle)
        {
            // B is the reference here, so the calculated normal is the direction that B pushes A,
            // but Collide() outputs the direction that A pushes B, so the normal is negated
            bool result = Physics::IntersectCirclePolygon(
                bodyB->position, bodyB->Radius,
                bodyA->position, bodyA->GetTransformedVertices2D(),
                o_normal, o_depth);

            o_normal = -o_normal;
            return result;
        }
    }
    else if (shapeTypeA == EShapeType::Circle)
    {
        if (shapeTypeB == EShapeType::Box)
        {
            return Physics::IntersectCirclePolygon(
                bodyA->position, bodyA->Radius,
                bodyB->position, bodyB->GetTransformedVertices2D(),
                o_normal, o_depth);
        }
        else if (shapeTypeB == EShapeType::Circle)
        {
            return Physics::IntersectCircles(bodyA->position, bodyA->Radius, bodyB->position, bodyB->Radius, o_normal, o_depth);
        }
    }
    return false;
}

bool eae6320::Physics::Collide(PhysicsBody2D* bodyA, PhysicsBody2D* bodyB)
{
    auto o_normal = Math::sVector2::Zero();
    float o_depth = 0;

    EShapeType shapeTypeA = bodyA->ShapeType;
    EShapeType shapeTypeB = bodyB->ShapeType;

    if (shapeTypeA == EShapeType::Box)
    {
        if (shapeTypeB == EShapeType::Box)
        {
            return Physics::IntersectPolygons(
                bodyA->position, bodyA->GetTransformedVertices2D(),
                bodyB->position, bodyB->GetTransformedVertices2D(),
                o_normal, o_depth);
        }
        else if (shapeTypeB == EShapeType::Circle)
        {
            // B is the reference here, so the calculated normal is the direction that B pushes A,
            // but Collide() outputs the direction that A pushes B, so the normal is negated
            bool result = Physics::IntersectCirclePolygon(
                bodyB->position, bodyB->Radius,
                bodyA->position, bodyA->GetTransformedVertices2D(),
                o_normal, o_depth);

            o_normal = -o_normal;
            return result;
        }
    }
    else if (shapeTypeA == EShapeType::Circle)
    {
        if (shapeTypeB == EShapeType::Box)
        {
            return Physics::IntersectCirclePolygon(
                bodyA->position, bodyA->Radius,
                bodyB->position, bodyB->GetTransformedVertices2D(),
                o_normal, o_depth);
        }
        else if (shapeTypeB == EShapeType::Circle)
        {
            return Physics::IntersectCircles(bodyA->position, bodyA->Radius, bodyB->position, bodyB->Radius, o_normal, o_depth);
        }
    }
    return false;
}


bool eae6320::Physics::IntersectCircles(const Math::sVector2& i_centerA, float i_radiusA, const Math::sVector2& i_centerB, float i_radiusB, Math::sVector2& o_normal, float& o_depth)
{
    float distance = i_centerA.Distance(i_centerB);
    float radii = i_radiusA + i_radiusB;

    if (distance >= radii)
    {
        return false;
    }

    o_normal = (i_centerB - i_centerA).GetNormalized();
    o_depth = radii - distance;

    return true;
}

bool eae6320::Physics::IntersectPolygons(const Math::sVector2& i_polygonCenterA, const std::vector<Math::sVector2>& i_verticesA,
    const Math::sVector2& i_polygonCenterB, const std::vector<Math::sVector2>& i_verticesB,
    Math::sVector2& o_normal, float& o_depth)
{
    o_normal = Math::sVector2::Zero();
    // The smallest overlap found so far (the depth needed to push the shapes apart)
    o_depth = FLT_MAX;
    for (unsigned int i = 0; i < i_verticesA.size(); i++)
    {
        Math::sVector2 va = i_verticesA[i];
        Math::sVector2 vb = i_verticesA[(i + 1) % i_verticesA.size()];

        Math::sVector2 edge = vb - va;
        Math::sVector2 axis = Math::sVector2(-edge.y, edge.x);
        axis = axis.GetNormalized();

        // Project both shapes onto the separating axis and compare the ranges:
        // the shapes only intersect if their projections overlap on every axis
        float minA, maxA, minB, maxB;
        ProjectVertices(i_verticesA, axis, minA, maxA);
        ProjectVertices(i_verticesB, axis, minB, maxB);

        if (minA >= maxB || minB >= maxA)
        {
            return false;
        }

        float axisdepth = std::min(maxB - minA, maxA - minB);

        if (axisdepth < o_depth)
        {
            o_depth = axisdepth;
            o_normal = axis;
        }
    }

    for (unsigned int i = 0; i < i_verticesB.size(); i++)
    {
        Math::sVector2 va = i_verticesB[i];
        Math::sVector2 vb = i_verticesB[(i + 1) % i_verticesB.size()];

        Math::sVector2 edge = vb - va;
        Math::sVector2 axis = Math::sVector2(-edge.y, edge.x);
        axis = axis.GetNormalized();

        // Project both shapes onto the separating axis and compare the ranges:
        // the shapes only intersect if their projections overlap on every axis
        float minA, maxA, minB, maxB;
        ProjectVertices(i_verticesA, axis, minA, maxA);
        ProjectVertices(i_verticesB, axis, minB, maxB);

        if (minA >= maxB || minB >= maxA)
        {
            return false;
        }

        float axisdepth = std::min(maxB - minA, maxA - minB);

        if (axisdepth < o_depth)
        {
            o_depth = axisdepth;
            o_normal = axis;
        }
    }

    Math::sVector2 direction = i_polygonCenterB - i_polygonCenterA;

    if (Math::Dot(direction, o_normal) < 0)
    {
        o_normal = -o_normal;
    }

    return true;
}

bool eae6320::Physics::IntersectCirclePolygon(const Math::sVector2& i_circleCenter, const float& i_circleRadius, const Math::sVector2& i_polygonCenter, const std::vector<Math::sVector2>& i_vertices, Math::sVector2& o_normal, float& o_depth)
{
    o_normal = Math::sVector2::Zero();
    // The smallest overlap found so far (the depth needed to push the shapes apart)
    o_depth = FLT_MAX;
    Math::sVector2 axis = Math::sVector2::Zero();
    float axisdepth = 0;
    float minA, maxA, minB, maxB;
    for (unsigned int i = 0; i < i_vertices.size(); i++)
    {
        Math::sVector2 va = i_vertices[i];
        Math::sVector2 vb = i_vertices[(i + 1) % i_vertices.size()];

        Math::sVector2 edge = vb - va;
        axis = Math::sVector2(-edge.y, edge.x);
        axis = axis.GetNormalized();

        // Project both shapes onto the separating axis and compare the ranges:
        // the shapes only intersect if their projections overlap on every axis
        ProjectVertices(i_vertices, axis, minA, maxA);
        ProjectCircle(i_circleCenter, i_circleRadius, axis, minB, maxB);

        if (minA >= maxB || minB >= maxA)
        {
            return false;
        }

        axisdepth = std::min(maxB - minA, maxA - minB);

        if (axisdepth < o_depth)
        {
            o_depth = axisdepth;
            o_normal = axis;
        }
    }

    int cpIndex = FindClosestPointOnPolygon(i_circleCenter, i_vertices);
    Math::sVector2 cp = i_vertices[cpIndex];

    axis = cp - i_circleCenter;
    axis = axis.GetNormalized();

    ProjectVertices(i_vertices, axis, minA, maxA);
    ProjectCircle(i_circleCenter, i_circleRadius, axis, minB, maxB);

    if (minA >= maxB || minB >= maxA)
    {
        return false;
    }

    axisdepth = std::min(maxB - minA, maxA - minB);

    if (axisdepth < o_depth)
    {
        o_depth = axisdepth;
        o_normal = axis;
    }

    Math::sVector2 direction = i_polygonCenter - i_circleCenter;

    if (Math::Dot(direction, o_normal) < 0)
    {
        o_normal = -o_normal;
    }

    return true;
}

void eae6320::Physics::PointSegmentDistance(const Math::sVector2& i_p, const Math::sVector2& i_a, const Math::sVector2& i_b, float& o_distSqr, Math::sVector2& o_cp)
{
    Math::sVector2 ab = i_b - i_a;
    Math::sVector2 ap = i_p - i_a;

    float proj = Math::Dot(ap, ab);
    float abLenSqr = ab.GetLengthSquared();
    float d = proj / abLenSqr;

    // If d <= 0, p projects onto the line before a, so the closest point on the segment ab is a
    if (d <= 0)
    {
        o_cp = i_a;
    }
    // The opposite case: p projects past b, so the closest point is b
    else if (d >= 1)
    {
        o_cp = i_b;
    }
    else
    {
        o_cp = i_a + ab * d;
    }

    o_distSqr = i_p.Distance(o_cp);
}

void ProjectVertices(const std::vector<eae6320::Math::sVector2>& i_vertices, const eae6320::Math::sVector2& i_axis, float& o_min, float& o_max)
{
    using namespace eae6320::Math;
    o_min = FLT_MAX;
    o_max = -FLT_MAX;

    for (unsigned int i = 0; i < i_vertices.size(); i++)
    {
        sVector2 v = i_vertices[i];
        float proj = Dot(v, i_axis);

        if (proj < o_min) o_min = proj;
        if (proj > o_max) o_max = proj;


    }
}

void ProjectCircle(const eae6320::Math::sVector2& i_center, const float& i_radius, eae6320::Math::sVector2& i_axis, float& o_min, float& o_max)
{
    using namespace eae6320::Math;
    sVector2 direction = i_axis.GetNormalized();
    sVector2 directionAndRadius = direction * i_radius;

    sVector2 p1 = i_center + directionAndRadius;
    sVector2 p2 = i_center - directionAndRadius;

    o_min = Dot(p1, i_axis);
    o_max = Dot(p2, i_axis);

    // 
    if (o_min > o_max)
    {
        float t = o_min;
        o_min = o_max;
        o_max = t;
    }
}

int FindClosestPointOnPolygon(const eae6320::Math::sVector2& i_circleCenter, const std::vector<eae6320::Math::sVector2>& i_vertices)
{
    using namespace eae6320::Math;
    int result = -1;
    float minDistance = FLT_MAX;
    for (unsigned int i = 0; i < i_vertices.size(); i++)
    {
        sVector2 v = i_vertices[i];
        float distance = v.Distance(i_circleCenter);
        if (distance < minDistance)
        {
            minDistance = distance;
            result = i;
        }
    }
    return result;
}

// =============================================
// Raycast Implementations
// =============================================

bool eae6320::Physics::RayIntersectAABB(const Math::sVector2& i_rayOrigin, const Math::sVector2& i_rayDir,
    float i_rayDistance, const AABB& i_aabb)
{
    // Slab method for ray-AABB intersection
    // Handle case where ray direction component is zero
    float tMinX, tMaxX, tMinY, tMaxY;

    if (std::abs(i_rayDir.x) < 0.0001f)
    {
        // Ray is parallel to YZ plane
        if (i_rayOrigin.x < i_aabb.Min.x || i_rayOrigin.x > i_aabb.Max.x)
            return false;
        tMinX = -FLT_MAX;
        tMaxX = FLT_MAX;
    }
    else
    {
        float invDirX = 1.0f / i_rayDir.x;
        float t1 = (i_aabb.Min.x - i_rayOrigin.x) * invDirX;
        float t2 = (i_aabb.Max.x - i_rayOrigin.x) * invDirX;

        tMinX = std::min(t1, t2);
        tMaxX = std::max(t1, t2);
    }

    if (std::abs(i_rayDir.y) < 0.0001f)
    {
        // Ray is parallel to XZ plane
        if (i_rayOrigin.y < i_aabb.Min.y || i_rayOrigin.y > i_aabb.Max.y)
            return false;
        tMinY = -FLT_MAX;
        tMaxY = FLT_MAX;
    }
    else
    {
        float invDirY = 1.0f / i_rayDir.y;
        float t1 = (i_aabb.Min.y - i_rayOrigin.y) * invDirY;
        float t2 = (i_aabb.Max.y - i_rayOrigin.y) * invDirY;

        tMinY = std::min(t1, t2);
        tMaxY = std::max(t1, t2);
    }

    float tNear = std::max(tMinX, tMinY);
    float tFar = std::min(tMaxX, tMaxY);

    // Check if ray intersects AABB and is within ray distance
    if (tNear > tFar || tFar < 0.0f || tNear > i_rayDistance)
        return false;

    return true;
}

bool eae6320::Physics::RayIntersectCircle(const Math::sVector2& i_rayOrigin, const Math::sVector2& i_rayDir,
    float i_rayDistance, const Math::sVector2& i_circleCenter, float i_radius,
    Math::sVector2& o_hitPoint, Math::sVector2& o_normal, float& o_distance)
{
    // Vector from ray origin to circle center
    Math::sVector2 oc = i_rayOrigin - i_circleCenter;

    // Solve quadratic equation: at^2 + bt + c = 0
    float a = Math::Dot(i_rayDir, i_rayDir);
    float b = 2.0f * Math::Dot(oc, i_rayDir);
    float c = Math::Dot(oc, oc) - i_radius * i_radius;

    float discriminant = b * b - 4.0f * a * c;

    // No intersection
    if (discriminant < 0.0f)
        return false;

    // Calculate the nearest intersection point
    float sqrtDisc = std::sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);

    // Use the nearest positive t value
    float t = t1;
    if (t < 0.0f)
        t = t2;

    // Check if intersection is within ray distance and positive
    if (t < 0.0f || t > i_rayDistance)
        return false;

    // Calculate hit point and normal
    o_hitPoint = i_rayOrigin + i_rayDir * t;
    o_normal = (o_hitPoint - i_circleCenter).GetNormalized();
    o_distance = t;

    return true;
}

bool eae6320::Physics::RayIntersectBox(const Math::sVector2& i_rayOrigin, const Math::sVector2& i_rayDir,
    float i_rayDistance, const std::vector<Math::sVector2>& i_vertices,
    Math::sVector2& o_hitPoint, Math::sVector2& o_normal, float& o_distance)
{
    if (i_vertices.size() < 3)
        return false;

    float minT = FLT_MAX;
    Math::sVector2 closestHitPoint;
    Math::sVector2 closestNormal;
    bool hasHit = false;

    // Test ray against each edge of the polygon
    for (unsigned int i = 0; i < i_vertices.size(); i++)
    {
        Math::sVector2 edgeStart = i_vertices[i];
        Math::sVector2 edgeEnd = i_vertices[(i + 1) % i_vertices.size()];

        Math::sVector2 edgeDir = edgeEnd - edgeStart;
        Math::sVector2 edgeNormal = Math::sVector2(-edgeDir.y, edgeDir.x).GetNormalized();

        // Ray-line segment intersection using parametric form
        // Ray: P = rayOrigin + t * rayDir
        // Line segment: Q = edgeStart + s * edgeDir, where 0 <= s <= 1

        Math::sVector2 originToEdge = edgeStart - i_rayOrigin;

        float denominator = Math::Cross(i_rayDir, edgeDir);

        // Check if ray and edge are parallel
        if (std::abs(denominator) < 0.0001f)
            continue;

        float t = Math::Cross(originToEdge, edgeDir) / denominator;
        float s = Math::Cross(originToEdge, i_rayDir) / denominator;

        // Check if intersection is valid
        if (t >= 0.0f && t <= i_rayDistance && s >= 0.0f && s <= 1.0f)
        {
            if (t < minT)
            {
                minT = t;
                closestHitPoint = i_rayOrigin + i_rayDir * t;
                closestNormal = edgeNormal;

                // Make sure normal points towards the ray origin
                if (Math::Dot(closestNormal, -i_rayDir) < 0.0f)
                    closestNormal = -closestNormal;

                hasHit = true;
            }
        }
    }

    if (hasHit)
    {
        o_hitPoint = closestHitPoint;
        o_normal = closestNormal;
        o_distance = minT;
        return true;
    }

    return false;
}