/*
	Simple 3D collision shapes and the tests between them

	These answer "did this thing touch that thing?" for games that move their objects themselves
	(with sRigidBodyState) and only need to know about contacts,
	rather than wanting a physics simulation to respond to them (bouncing, friction, etc.).

	Only two shapes are supported:
		* An axis-aligned box (sAabb3D), which can't rotate but is very cheap to test
		* A sphere (sSphere3D), which looks the same from every direction so rotation doesn't matter
*/

#ifndef EAE6320_PHYSICS_COLLISION3D_H
#define EAE6320_PHYSICS_COLLISION3D_H

// Includes
//=========

#include <Engine/Math/sVector.h>

// Struct Declarations
//====================

namespace eae6320
{
	namespace Physics
	{
		// An axis-aligned bounding box:
		// a box whose sides always line up with the world's x, y, and z axes
		struct sAabb3D
		{
			Math::sVector center;
			// Half of the box's size on each axis (the distance from the center to each side)
			Math::sVector halfExtents;

			Math::sVector GetMinimum() const { return center - halfExtents; }
			Math::sVector GetMaximum() const { return center + halfExtents; }
		};

		struct sSphere3D
		{
			Math::sVector center;
			float radius = 0.0f;
		};
	}
}

// Interface
//==========

namespace eae6320
{
	namespace Physics
	{
		namespace Collision3D
		{
			// Overlap tests answer "are these touching right now?"
			//----------------------------------------------------

			bool DoAabbsOverlap( const sAabb3D& i_a, const sAabb3D& i_b );
			bool DoesSphereOverlapAabb( const sSphere3D& i_sphere, const sAabb3D& i_box );

			// Sweep tests answer "did this touch that at any point while it moved?"
			//---------------------------------------------------------------------

			// An overlap test only looks at where things are at the end of a frame,
			// so something small and fast can be on one side of a thin object in one frame
			// and on the other side in the next frame without ever "overlapping" it ("tunneling").
			// A sweep tests the whole path that was traveled during the frame instead.
			//
			// The sphere moves from i_sphere_start.center to (i_sphere_start.center + i_displacement).
			// If it touches the box along the way this returns true,
			// and o_timeOfImpact is how far along the path the first touch happened
			// (0 is the start of the path and 1 is the end).
			// If the sphere already overlaps the box at the start o_timeOfImpact is 0.
			bool SweepSphereAgainstAabb( const sSphere3D& i_sphere_start, const Math::sVector& i_displacement,
				const sAabb3D& i_box, float& o_timeOfImpact );
		}
	}
}

#endif	// EAE6320_PHYSICS_COLLISION3D_H
