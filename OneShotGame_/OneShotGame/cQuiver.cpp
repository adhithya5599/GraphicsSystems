// Includes
//=========

#include "cQuiver.h"

#include "sGameAssets.h"

#include <algorithm>
#include <cmath>
#include <Engine/Math/cQuaternion.h>

// Interface
//==========

void eae6320::OneShot::cQuiver::Initialize( const sGameAssets& i_assets )
{
	m_quiver.SetMesh( i_assets.quiverMesh );
	m_quiver.SetEffect( i_assets.litEffect );
	m_quiver.SetTexture( i_assets.whiteTexture );
	m_quiver.GetRigidBodyState().position = Settings::quiverPosition;
	m_arrow.SetMesh( i_assets.arrowMesh );
	m_arrow.SetEffect( i_assets.litEffect );
	m_arrow.SetTexture( i_assets.whiteTexture );

	// The arrows stand point-down in the tube in a small circle, each leaning out a little so that their fletching fans out.
	// The arrow mesh points down its -z axis (its nock is at the origin and its tip is 0.75 m along -z),
	// so each arrow is turned to point its -z axis along the direction from its nock to its tip.
	constexpr float pi = 3.14159265f;
	const auto lean = Settings::quiverArrowLeanDegrees * pi / 180.0f;
	for ( size_t i = 0; i < m_arrowTransforms.size(); ++i )
	{
		const auto angle = ( ( 2.0f * pi * static_cast<float>( i ) ) / static_cast<float>( m_arrowTransforms.size() ) ) + 0.26f;
		const Math::sVector outward( std::cos( angle ), 0.0f, std::sin( angle ) );
		// The nocks aren't all at the same height (real arrows never sit perfectly level)
		const auto nockHeight = Settings::quiverNockHeight + ( 0.02f * static_cast<float>( i % 3 ) );
		const auto nock = Settings::quiverPosition + ( outward * Settings::quiverArrowSpread ) + Math::sVector( 0.0f, nockHeight, 0.0f );
		// Leaning out at the top means pointing down and slightly in
		const auto nockToTip = Math::sVector( 0.0f, -std::cos( lean ), 0.0f ) - ( outward * std::sin( lean ) );
		// (LookRotation() turns -z to face the first direction; the second is any direction that isn't parallel to it)
		m_arrowTransforms[i] = Math::cMatrix_transformation( Math::cQuaternion::LookRotation( nockToTip, outward ), nock );
	}
}

void eae6320::OneShot::cQuiver::ReleaseAssets()
{
	m_quiver.ReleaseAssets();
	m_arrow.ReleaseAssets();
}

void eae6320::OneShot::cQuiver::SubmitToBeRendered( const unsigned int i_arrowCount )
{
	// (The quiver never moves, so there is nothing to extrapolate)
	m_quiver.SubmitToBeRendered( 0.0f );
	const auto arrowCount = std::min( static_cast<size_t>( i_arrowCount ), m_arrowTransforms.size() );
	for ( size_t i = 0; i < arrowCount; ++i )
	{
		m_arrow.SubmitToBeRendered( m_arrowTransforms[i] );
	}
}
