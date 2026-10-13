// Includes
//=========

#include "cArrow.h"

#include "GameplaySettings.h"
#include "sGameAssets.h"

#include <algorithm>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/cQuaternion.h>

// Interface
//==========

// Initialize / Clean Up
//----------------------

void eae6320::OneShot::cArrow::Initialize( const sGameAssets& i_assets )
{
	m_gameObject.SetMesh( i_assets.arrowMesh );
	m_gameObject.SetEffect( i_assets.litEffect );
	m_gameObject.SetTexture( i_assets.whiteTexture );
	Reset();
}

void eae6320::OneShot::cArrow::Reset()
{
	auto& rigidBodyState = m_gameObject.GetRigidBodyState();
	rigidBodyState.position = Settings::arrowRestPosition;
	rigidBodyState.velocity = Math::sVector();
	m_position_startOfLastUpdate = rigidBodyState.position;
	m_isFlying = false;
	m_isFalling = false;
}

// Flight
//-------

void eae6320::OneShot::cArrow::Launch()
{
	m_isFlying = true;
	m_gameObject.GetRigidBodyState().velocity = Math::sVector( 0.0f, 0.0f, -Settings::arrowSpeed_normal );
}

void eae6320::OneShot::cArrow::Stop()
{
	m_isFlying = false;
	// Without a velocity the renderer won't extrapolate it past where it stopped
	m_gameObject.GetRigidBodyState().velocity = Math::sVector();
}

void eae6320::OneShot::cArrow::Deflect()
{
	Stop();
	m_isFalling = true;
	m_gameObject.GetRigidBodyState().velocity = Math::sVector( 0.0f, Settings::arrowDeflectSpeedUp, Settings::arrowDeflectSpeedBack );
}

void eae6320::OneShot::cArrow::StopPartwayThroughLastUpdate( const float i_fractionOfLastUpdate )
{
	auto& position = m_gameObject.GetRigidBodyState().position;
	position = m_position_startOfLastUpdate + ( ( position - m_position_startOfLastUpdate ) * i_fractionOfLastUpdate );
	Stop();
}

void eae6320::OneShot::cArrow::Steer( const float i_horizontalInput, const float i_verticalInput, const bool i_isBoosting, const float i_windDrift )
{
	if ( !m_isFlying )
	{
		return;
	}
	const auto forwardSpeed = i_isBoosting ? Settings::arrowSpeed_boosted : Settings::arrowSpeed_normal;
	m_gameObject.GetRigidBodyState().velocity = Math::sVector(
		( i_horizontalInput * Settings::arrowSteeringSpeed ) + i_windDrift,
		i_verticalInput * Settings::arrowSteeringSpeed,
		-forwardSpeed );
	// Steering happens after the update that clamped the position,
	// so steering into an edge must be stopped here as well
	RemoveVelocityPastSteeringLimits();
}

void eae6320::OneShot::cArrow::Update( const float i_elapsedSecondCount_sinceLastUpdate )
{
	auto& rigidBodyState = m_gameObject.GetRigidBodyState();
	m_position_startOfLastUpdate = rigidBodyState.position;
	if ( m_isFalling )
	{
		// Gravity pulls it down until it lands on the grass
		rigidBodyState.velocity.y -= Settings::gravity * i_elapsedSecondCount_sinceLastUpdate;
		rigidBodyState.Update( i_elapsedSecondCount_sinceLastUpdate );
		if ( rigidBodyState.position.y <= Settings::arrowHeightOnGround )
		{
			rigidBodyState.position.y = Settings::arrowHeightOnGround;
			rigidBodyState.velocity = Math::sVector();
			m_isFalling = false;
		}
		return;
	}
	if ( !m_isFlying )
	{
		return;
	}

	rigidBodyState.Update( i_elapsedSecondCount_sinceLastUpdate );

	// Keep the arrow inside the area that it can be steered in
	auto& position = rigidBodyState.position;
	position.x = std::clamp( position.x, Settings::arrowSteeringLimit_minimum.x, Settings::arrowSteeringLimit_maximum.x );
	position.y = std::clamp( position.y, Settings::arrowSteeringLimit_minimum.y, Settings::arrowSteeringLimit_maximum.y );
	RemoveVelocityPastSteeringLimits();
}

// Render
//-------

void eae6320::OneShot::cArrow::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	if ( m_isFalling )
	{
		// A falling arrow is extrapolated like anything else, but never below the grass
		// (just before it lands it is falling fast enough that the extrapolation would draw it partly underground)
		const auto& rigidBodyState = m_gameObject.GetRigidBodyState();
		auto position = rigidBodyState.position + ( rigidBodyState.velocity * i_elapsedSecondCount_sinceLastSimulationUpdate );
		position.y = std::max( position.y, Settings::arrowHeightOnGround );
		m_gameObject.SubmitToBeRendered( Math::cMatrix_transformation( Math::cQuaternion(), position ) );
	}
	else
	{
		m_gameObject.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	}
}

// Implementation
//===============

void eae6320::OneShot::cArrow::RemoveVelocityPastSteeringLimits()
{
	// When the arrow is at an edge its velocity toward that edge is removed:
	// otherwise the renderer, which extrapolates positions using the velocity,
	// would draw it past the edge and then snap it back every update (a visible jitter)
	auto& rigidBodyState = m_gameObject.GetRigidBodyState();
	const auto& position = rigidBodyState.position;
	auto& velocity = rigidBodyState.velocity;
	const auto& minimum = Settings::arrowSteeringLimit_minimum;
	const auto& maximum = Settings::arrowSteeringLimit_maximum;
	if ( ( ( position.x <= minimum.x ) && ( velocity.x < 0.0f ) ) || ( ( position.x >= maximum.x ) && ( velocity.x > 0.0f ) ) )
	{
		velocity.x = 0.0f;
	}
	if ( ( ( position.y <= minimum.y ) && ( velocity.y < 0.0f ) ) || ( ( position.y >= maximum.y ) && ( velocity.y > 0.0f ) ) )
	{
		velocity.y = 0.0f;
	}
}

// Collision
//----------

eae6320::Physics::sSphere3D eae6320::OneShot::cArrow::GetTipAtStartOfLastUpdate() const
{
	// The arrow never rotates, so its local offsets can just be added to its position
	return Physics::sSphere3D{ m_position_startOfLastUpdate + Settings::arrowTip_local, Settings::arrowTipRadius };
}

eae6320::Math::sVector eae6320::OneShot::cArrow::GetTipMovementDuringLastUpdate() const
{
	return GetPosition() - m_position_startOfLastUpdate;
}

eae6320::Math::sVector eae6320::OneShot::cArrow::GetTipPosition() const
{
	return GetPosition() + Settings::arrowTip_local;
}

eae6320::Physics::sAabb3D eae6320::OneShot::cArrow::GetShaftCollider() const
{
	return Physics::sAabb3D{ GetPosition() + Settings::arrowShaftCenter_local, Settings::arrowShaftHalfExtents };
}
