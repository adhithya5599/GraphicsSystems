// Includes
//=========

#include "cTarget.h"

#include "cArrow.h"
#include "GameplaySettings.h"
#include "sGameAssets.h"

#include <cmath>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/cQuaternion.h>

// Interface
//==========

void eae6320::OneShot::cTarget::Initialize( const sGameAssets& i_assets )
{
	for ( size_t i = 0; i < m_targets.size(); ++i )
	{
		auto& target = m_targets[i];
		target.SetMesh( i_assets.targetMesh );
		target.SetEffect( i_assets.litEffect );
		target.SetTexture( i_assets.targetTexture );
		auto position = Settings::targetPosition;
		if ( i > 0 )
		{
			position.x = Settings::neighborTargetXs[i - 1];
		}
		target.GetRigidBodyState().position = position;
	}
	m_stuckArrow.SetMesh( i_assets.arrowMesh );
	m_stuckArrow.SetEffect( i_assets.litEffect );
	m_stuckArrow.SetTexture( i_assets.whiteTexture );
	m_stuckArrowNockPositions.reserve( Settings::arrowsPerRound );
}

void eae6320::OneShot::cTarget::ReleaseAssets()
{
	for ( auto& target : m_targets )
	{
		target.ReleaseAssets();
	}
	m_stuckArrow.ReleaseAssets();
}

bool eae6320::OneShot::cTarget::IsHitBy( const cArrow& i_arrow, float& o_timeOfImpact, unsigned int& o_score ) const
{
	// The boss leans back, so its front isn't lined up with the world's axes and a box collider would be a poor fit.
	// Instead the tip's path during the last update is tested against the tilted plane of the face:
	//	* The plane goes through the face's center and its normal leans up by the tilt angle
	//	* "up" along the face leans back by the same angle
	const auto tilt = Settings::targetTiltDegrees * 3.14159265f / 180.0f;
	const auto faceCenter = Settings::targetPosition + Math::sVector( 0.0f, Settings::targetFaceCenterHeight, 0.0f );
	const Math::sVector normal( 0.0f, std::sin( tilt ), std::cos( tilt ) );
	const Math::sVector faceUp( 0.0f, std::cos( tilt ), -std::sin( tilt ) );
	const Math::sVector faceRight( 1.0f, 0.0f, 0.0f );

	const auto start = i_arrow.GetTipAtStartOfLastUpdate().center;
	const auto movement = i_arrow.GetTipMovementDuringLastUpdate();
	// How far in front of the face the tip was at the start and end of the update
	// (positive is in front, toward the archer)
	const auto distance_start = Dot( start - faceCenter, normal );
	const auto distance_end = Dot( ( start + movement ) - faceCenter, normal );
	if ( ( distance_start < 0.0f ) || ( distance_end > 0.0f ) )
	{
		// The tip didn't cross from in front of the plane to behind it during this update
		return false;
	}
	// Where along the path the tip crossed the plane (the distance shrinks linearly along the path)
	const auto timeOfImpact = ( distance_start == distance_end ) ? 0.0f : distance_start / ( distance_start - distance_end );
	const auto impact = start + ( movement * timeOfImpact );
	// Where on the face it hit, measured across and up the face from its center
	const auto across = Dot( impact - faceCenter, faceRight );
	const auto up = Dot( impact - faceCenter, faceUp );
	if ( ( std::abs( across ) > Settings::targetBossHalfSize ) || ( std::abs( up ) > Settings::targetBossHalfSize ) )
	{
		// It went past the boss
		return false;
	}
	o_timeOfImpact = timeOfImpact;
	const auto distanceFromCenter = std::sqrt( ( across * across ) + ( up * up ) );
	constexpr unsigned int ringCount = 10;
	const auto ringWidth = Settings::targetFaceRadius / static_cast<float>( ringCount );
	const auto ringIndex = static_cast<unsigned int>( distanceFromCenter / ringWidth );
	o_score = ( ringIndex < ringCount ) ? ( ringCount - ringIndex ) : 0;
	return true;
}

void eae6320::OneShot::cTarget::StickArrow( const Math::sVector& i_nockPosition )
{
	m_stuckArrowNockPositions.push_back( i_nockPosition );
}

void eae6320::OneShot::cTarget::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	for ( auto& target : m_targets )
	{
		target.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	}
	for ( const auto& nockPosition : m_stuckArrowNockPositions )
	{
		m_stuckArrow.SubmitToBeRendered( Math::cMatrix_transformation( Math::cQuaternion(), nockPosition ) );
	}
}
