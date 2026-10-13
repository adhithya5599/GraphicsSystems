// Includes
//=========

#include "cCrowd.h"

#include "GameplaySettings.h"

#include <algorithm>
#include <cmath>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/cQuaternion.h>

// Interface
//==========

void eae6320::OneShot::cCrowd::Initialize( const sGameAssets& i_assets )
{
	for ( size_t i = 0; i < m_groups.size(); ++i )
	{
		// The crowd's meshes are in world space, like the rest of the grandstand, so each group only ever moves up and down
		m_groups[i].SetMesh( i_assets.crowdMeshes[i] );
		m_groups[i].SetEffect( i_assets.litEffect );
		m_groups[i].SetTexture( i_assets.whiteTexture );
	}
	Settle();
}

void eae6320::OneShot::cCrowd::ReleaseAssets()
{
	for ( auto& group : m_groups )
	{
		group.ReleaseAssets();
	}
}

void eae6320::OneShot::cCrowd::Cheer( const float i_excitement )
{
	m_excitement = std::clamp( i_excitement, 0.0f, 1.0f );
	m_cheerSecondCount = 0.0f;
}

void eae6320::OneShot::cCrowd::Settle()
{
	m_excitement = 0.0f;
	m_cheerSecondCount = Settings::crowdCheerDuration;
	for ( auto& group : m_groups )
	{
		auto& rigidBodyState = group.GetRigidBodyState();
		rigidBodyState.position = Math::sVector();
		rigidBodyState.velocity = Math::sVector();
	}
}

void eae6320::OneShot::cCrowd::Update( const float i_elapsedSecondCount_sinceLastUpdate )
{
	if ( m_cheerSecondCount >= Settings::crowdCheerDuration )
	{
		return;
	}
	m_cheerSecondCount = std::min( m_cheerSecondCount + i_elapsedSecondCount_sinceLastUpdate, Settings::crowdCheerDuration );
	// Each group's height is set directly from the time in the cheer,
	// and its velocity is how fast that height is changing right now
	// (the velocity is only used to extrapolate the height between simulation updates so that the hops look smooth when drawn)
	constexpr float derivativeStep = 1.0f / 240.0f;
	for ( size_t i = 0; i < m_groups.size(); ++i )
	{
		auto& rigidBodyState = m_groups[i].GetRigidBodyState();
		const auto height = CalculateHopHeight( i, m_cheerSecondCount );
		const auto heightSoon = CalculateHopHeight( i, m_cheerSecondCount + derivativeStep );
		rigidBodyState.position = Math::sVector( 0.0f, height, 0.0f );
		rigidBodyState.velocity = Math::sVector( 0.0f, ( heightSoon - height ) / derivativeStep, 0.0f );
	}
}

void eae6320::OneShot::cCrowd::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	for ( auto& group : m_groups )
	{
		// Each group is extrapolated like anything else, but never below its seats:
		// just before a hop lands it is still moving down, and the extrapolation doesn't know that the hop is about to stop
		const auto& rigidBodyState = group.GetRigidBodyState();
		auto position = rigidBodyState.position + ( rigidBodyState.velocity * i_elapsedSecondCount_sinceLastSimulationUpdate );
		position.y = std::max( position.y, 0.0f );
		group.SubmitToBeRendered( Math::cMatrix_transformation( Math::cQuaternion(), position ) );
	}
}

// Implementation
//===============

float eae6320::OneShot::cCrowd::CalculateHopHeight( const size_t i_groupIndex, const float i_cheerSecondCount ) const
{
	if ( i_cheerSecondCount >= Settings::crowdCheerDuration )
	{
		return 0.0f;
	}
	constexpr float pi = 3.14159265f;
	// The hops shrink as the cheer dies down
	const auto fade = 1.0f - ( i_cheerSecondCount / Settings::crowdCheerDuration );
	// Each group is a third of a hop behind the one before it
	const auto phase = static_cast<float>( i_groupIndex ) / static_cast<float>( sGameAssets::crowdGroupCount );
	// sin² goes up and comes back down once per hop, never goes below the seat,
	// and eases in and out at the bottom of each hop (so it looks like a jump rather than a bounce)
	const auto wave = std::sin( pi * ( ( Settings::crowdHopsPerSecond * i_cheerSecondCount ) + phase ) );
	const auto hop = wave * wave;
	return Settings::crowdHopHeight * m_excitement * fade * hop;
}
