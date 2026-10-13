// Includes
//=========

#include "cWind.h"

#include "GameplaySettings.h"
#include "sGameAssets.h"

#include <algorithm>
#include <cmath>

// Interface
//==========

void eae6320::OneShot::cWind::Initialize( const sGameAssets& i_assets )
{
	static_assert( std::tuple_size<decltype( Settings::flagPolePositions )>::value == std::tuple_size<decltype( m_flags )>::value,
		"There must be one flag for every flag pole position" );
	for ( size_t i = 0; i < m_flags.size(); ++i )
	{
		auto& flag = m_flags[i];
		flag.pole.SetMesh( i_assets.flagPoleMesh );
		flag.pole.SetEffect( i_assets.litEffect );
		flag.pole.SetTexture( i_assets.whiteTexture );
		flag.pole.GetRigidBodyState().position = Settings::flagPolePositions[i];
		flag.cloth.SetMesh( i_assets.flagMesh );
		flag.cloth.SetEffect( i_assets.litEffect );
		flag.cloth.SetTexture( i_assets.whiteTexture );
		flag.cloth.GetRigidBodyState().position = Settings::flagPolePositions[i];
	}
	m_elapsedSecondCount = 0.0f;
	Update( 0.0f );
}

void eae6320::OneShot::cWind::ReleaseAssets()
{
	for ( auto& flag : m_flags )
	{
		flag.pole.ReleaseAssets();
		flag.cloth.ReleaseAssets();
	}
}

void eae6320::OneShot::cWind::Update( const float i_elapsedSecondCount_sinceLastUpdate )
{
	m_elapsedSecondCount += i_elapsedSecondCount_sinceLastUpdate;
	constexpr float twoPi = 6.28318531f;
	m_speed = ( Settings::windSlowAmplitude * std::sin( twoPi * m_elapsedSecondCount / Settings::windSlowPeriodInSeconds ) )
		+ ( Settings::windFastAmplitude * std::sin( ( twoPi * m_elapsedSecondCount / Settings::windFastPeriodInSeconds ) + 1.3f ) );
	UpdateFlags();
}

void eae6320::OneShot::cWind::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	for ( auto& flag : m_flags )
	{
		flag.pole.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
		flag.cloth.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	}
}

// Implementation
//===============

void eae6320::OneShot::cWind::UpdateFlags()
{
	// The flag mesh streams out along +x from its pole.
	// It is turned around (half a turn around the pole) when the wind blows toward -x,
	// and stretched along its length by how strong the wind is: a light wind leaves it hanging short,
	// a strong one stretches it all the way out.
	const auto strength = std::min( std::abs( m_speed ) / Settings::windSpeedForFullFlag, 1.0f );
	const auto stretch = 0.3f + ( 0.7f * strength );
	const auto orientation = ( m_speed >= 0.0f ) ? Math::cQuaternion() : Math::cQuaternion( 3.14159265f, Math::sVector( 0.0f, 1.0f, 0.0f ) );
	for ( auto& flag : m_flags )
	{
		flag.cloth.GetRigidBodyState().orientation = orientation;
		flag.cloth.SetScale( Math::sVector( stretch, 1.0f, 1.0f ) );
	}
}
