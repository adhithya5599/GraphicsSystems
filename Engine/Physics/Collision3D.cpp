// Includes
//=========

#include "Collision3D.h"

#include <algorithm>
#include <cmath>

// Helper Declarations
//====================

namespace
{
	// sVector stores its components as named members (x, y, z) rather than as an array,
	// so this makes it possible to write one loop for all three axes
	float GetComponent( const eae6320::Math::sVector& i_vector, const unsigned int i_axis );
}

// Interface
//==========

bool eae6320::Physics::Collision3D::DoAabbsOverlap( const sAabb3D& i_a, const sAabb3D& i_b )
{
	// Two boxes that can't rotate overlap if and only if their ranges overlap on every axis.
	// On each axis the ranges overlap if the distance between the centers
	// is no more than the two half sizes added together.
	for ( unsigned int axis = 0; axis < 3; ++axis )
	{
		const auto distanceBetweenCenters = std::abs( GetComponent( i_a.center, axis ) - GetComponent( i_b.center, axis ) );
		const auto combinedHalfExtents = GetComponent( i_a.halfExtents, axis ) + GetComponent( i_b.halfExtents, axis );
		if ( distanceBetweenCenters > combinedHalfExtents )
		{
			// There is a gap on this axis, so the boxes can't be touching
			return false;
		}
	}
	return true;
}

bool eae6320::Physics::Collision3D::DoesSphereOverlapAabb( const sSphere3D& i_sphere, const sAabb3D& i_box )
{
	// Find the point inside the box that is closest to the sphere's center
	// (clamping the center to the box on each axis),
	// and then the sphere touches the box if that point is within the sphere's radius
	const auto boxMinimum = i_box.GetMinimum();
	const auto boxMaximum = i_box.GetMaximum();
	float distanceSquared = 0.0f;
	for ( unsigned int axis = 0; axis < 3; ++axis )
	{
		const auto center = GetComponent( i_sphere.center, axis );
		const auto closest = std::clamp( center, GetComponent( boxMinimum, axis ), GetComponent( boxMaximum, axis ) );
		const auto delta = center - closest;
		distanceSquared += delta * delta;
	}
	// Comparing squared distances avoids calculating a square root
	return distanceSquared <= ( i_sphere.radius * i_sphere.radius );
}

bool eae6320::Physics::Collision3D::SweepSphereAgainstAabb( const sSphere3D& i_sphere_start, const Math::sVector& i_displacement,
	const sAabb3D& i_box, float& o_timeOfImpact )
{
	// Moving a sphere against a box is the same as moving a single point (the sphere's center)
	// against the box grown by the sphere's radius on every side.
	// (Strictly speaking the grown shape should have rounded edges and corners;
	// using a box means that a sphere that passes very close to a corner can count as a hit
	// when it would actually have just missed.
	// That is a deliberate trade-off: the test stays simple and cheap,
	// and the error is never bigger than the radius, which is small for the things this is used for.)
	const sAabb3D grownBox{ i_box.center, i_box.halfExtents + i_sphere_start.radius };
	const auto boxMinimum = grownBox.GetMinimum();
	const auto boxMaximum = grownBox.GetMaximum();

	// The point travels along start + (t * displacement) for t from 0 to 1.
	// On each axis there is a range of t where the point is between the two sides of the box ("slabs");
	// the point is inside the box when it is inside all three ranges at the same time,
	// so the first touch is the latest time that it enters any range,
	// as long as that is earlier than the first time that it leaves any range.
	float timeOfEntry = 0.0f;
	float timeOfExit = 1.0f;
	for ( unsigned int axis = 0; axis < 3; ++axis )
	{
		const auto start = GetComponent( i_sphere_start.center, axis );
		const auto displacement = GetComponent( i_displacement, axis );
		const auto minimum = GetComponent( boxMinimum, axis );
		const auto maximum = GetComponent( boxMaximum, axis );

		constexpr float notMovingThreshold = 1.0e-8f;
		if ( std::abs( displacement ) < notMovingThreshold )
		{
			// The point isn't moving on this axis,
			// so it is either always between the sides or never is
			if ( ( start < minimum ) || ( start > maximum ) )
			{
				return false;
			}
		}
		else
		{
			auto timeOfEntry_axis = ( minimum - start ) / displacement;
			auto timeOfExit_axis = ( maximum - start ) / displacement;
			if ( timeOfEntry_axis > timeOfExit_axis )
			{
				// Moving in the negative direction reaches the maximum side first
				std::swap( timeOfEntry_axis, timeOfExit_axis );
			}
			timeOfEntry = std::max( timeOfEntry, timeOfEntry_axis );
			timeOfExit = std::min( timeOfExit, timeOfExit_axis );
			if ( timeOfEntry > timeOfExit )
			{
				// The ranges don't overlap, so the point is never inside on all three axes at once
				return false;
			}
		}
	}
	o_timeOfImpact = timeOfEntry;
	return true;
}

// Helper Definitions
//===================

namespace
{
	float GetComponent( const eae6320::Math::sVector& i_vector, const unsigned int i_axis )
	{
		return ( i_axis == 0 ) ? i_vector.x : ( ( i_axis == 1 ) ? i_vector.y : i_vector.z );
	}
}
