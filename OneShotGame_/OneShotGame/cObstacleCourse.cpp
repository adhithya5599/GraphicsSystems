// Includes
//=========

#include "cObstacleCourse.h"

#include "cArrow.h"
#include "sGameAssets.h"

#include <Engine/Physics/Collision3D.h>

#include <algorithm>
#include <cmath>

// Helper Declarations
//====================

namespace
{
	// Returns a unit vector along the given axis
	eae6320::Math::sVector GetDirection( const eae6320::OneShot::Settings::eObstacleMotionAxis i_axis );
}

// Interface
//==========

// Initialize / Clean Up
//----------------------

void eae6320::OneShot::cObstacleCourse::Initialize( const sGameAssets& i_assets )
{
	m_obstacles.clear();
	m_obstacles.reserve( Settings::obstacleLayouts.size() );
	for ( const auto& layout : Settings::obstacleLayouts )
	{
		// Every obstacle shares the same mesh, effect, and texture
		// (each one just adds its own reference)
		sObstacle obstacle;
		obstacle.gameObject.SetMesh( i_assets.obstacleMesh );
		obstacle.gameObject.SetEffect( i_assets.litEffect );
		obstacle.gameObject.SetTexture( i_assets.obstacleTexture );
		obstacle.layout = layout;
		m_obstacles.push_back( std::move( obstacle ) );
	}
	m_elapsedSecondCount = 0.0f;
	MoveObstacles();
}

void eae6320::OneShot::cObstacleCourse::ReleaseAssets()
{
	for ( auto& obstacle : m_obstacles )
	{
		obstacle.gameObject.ReleaseAssets();
	}
}

// Update
//-------

void eae6320::OneShot::cObstacleCourse::Update( const float i_elapsedSecondCount_sinceLastUpdate )
{
	m_elapsedSecondCount += i_elapsedSecondCount_sinceLastUpdate;
	MoveObstacles();
}

bool eae6320::OneShot::cObstacleCourse::IsHitBy( const cArrow& i_arrow, float& o_timeOfImpact ) const
{
	auto isHit = false;
	o_timeOfImpact = 1.0f;
	const auto tipAtStart = i_arrow.GetTipAtStartOfLastUpdate();
	const auto tipMovement = i_arrow.GetTipMovementDuringLastUpdate();
	const auto shaft = i_arrow.GetShaftCollider();
	for ( const auto& obstacle : m_obstacles )
	{
		const Physics::sAabb3D obstacleCollider{
			obstacle.gameObject.GetRigidBodyState().position + Settings::obstacleColliderCenter_local,
			Settings::obstacleColliderHalfExtents };
		// The tip is swept along its path so that it can't skip through an obstacle between updates
		// (and if more than one obstacle is touched, the earliest touch is the one that counts)
		float timeOfImpact;
		if ( Physics::Collision3D::SweepSphereAgainstAabb( tipAtStart, tipMovement, obstacleCollider, timeOfImpact ) )
		{
			isHit = true;
			o_timeOfImpact = std::min( o_timeOfImpact, timeOfImpact );
		}
		// The shaft is checked where it is now so that clipping an obstacle while steering also counts
		// (there's no sweep for the shaft, so this counts as touching at the end of the update)
		else if ( Physics::Collision3D::DoAabbsOverlap( shaft, obstacleCollider ) )
		{
			isHit = true;
		}
	}
	return isHit;
}

// Render
//-------

void eae6320::OneShot::cObstacleCourse::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	for ( auto& obstacle : m_obstacles )
	{
		obstacle.gameObject.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	}
}

// Implementation
//===============

void eae6320::OneShot::cObstacleCourse::MoveObstacles()
{
	for ( auto& obstacle : m_obstacles )
	{
		// Each obstacle swings along its axis: offset = amplitude * sin( frequency * time ).
		// The position is set directly from the time (rather than integrated from a velocity)
		// so that the obstacles can never drift away from their paths,
		// but the velocity is still set (the derivative of the position: amplitude * frequency * cos( frequency * time ))
		// because the renderer uses it to extrapolate between simulation updates.
		// (The original game only set the positions,
		// so between the 15-per-second simulation updates the obstacles stood still and then jumped.)
		const auto& layout = obstacle.layout;
		const auto direction = GetDirection( layout.motionAxis );
		const auto phase = layout.angularFrequency * m_elapsedSecondCount;
		auto& rigidBodyState = obstacle.gameObject.GetRigidBodyState();
		rigidBodyState.position = layout.position + ( direction * ( layout.amplitude * std::sin( phase ) ) );
		rigidBodyState.velocity = direction * ( layout.amplitude * layout.angularFrequency * std::cos( phase ) );
	}
}

// Helper Definitions
//===================

namespace
{
	eae6320::Math::sVector GetDirection( const eae6320::OneShot::Settings::eObstacleMotionAxis i_axis )
	{
		switch ( i_axis )
		{
		case eae6320::OneShot::Settings::eObstacleMotionAxis::X: return eae6320::Math::sVector( 1.0f, 0.0f, 0.0f );
		case eae6320::OneShot::Settings::eObstacleMotionAxis::Y: return eae6320::Math::sVector( 0.0f, 1.0f, 0.0f );
		default: return eae6320::Math::sVector( 0.0f, 0.0f, 1.0f );
		}
	}
}
