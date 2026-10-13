// Includes
//=========

#include "cTimingLight.h"

#include "GameplaySettings.h"
#include "sGameAssets.h"

#include <Engine/Math/cQuaternion.h>

// Interface
//==========

void eae6320::OneShot::cTimingLight::Initialize( const sGameAssets& i_assets )
{
	{
		const auto yaw = Settings::TimingLight::yawDegrees * 3.14159265f / 180.0f;
		m_transform_localToWorld = Math::cMatrix_transformation( Math::cQuaternion( yaw, Math::sVector( 0.0f, 1.0f, 0.0f ) ),
			Settings::TimingLight::position );
	}
	m_frame.SetMesh( i_assets.timingLightFrameMesh );
	m_frame.SetEffect( i_assets.litEffect );
	m_frame.SetTexture( i_assets.whiteTexture );
	// The lamps and the word give off their own light, so they are drawn unlit
	m_redLamp.SetMesh( i_assets.timingLightRedMesh );
	m_redLamp.SetEffect( i_assets.unlitEffect );
	m_redLamp.SetTexture( i_assets.whiteTexture );
	m_greenLamp.SetMesh( i_assets.timingLightGreenMesh );
	m_greenLamp.SetEffect( i_assets.unlitEffect );
	m_greenLamp.SetTexture( i_assets.whiteTexture );
	// (The word uses the big screen's texture, which has the same dot-matrix letters)
	m_label.SetMesh( i_assets.timingLightLabelMesh );
	m_label.SetEffect( i_assets.unlitEffect );
	m_label.SetTexture( i_assets.scoreboardTexture );
}

void eae6320::OneShot::cTimingLight::ReleaseAssets()
{
	m_frame.ReleaseAssets();
	m_redLamp.ReleaseAssets();
	m_greenLamp.ReleaseAssets();
	m_label.ReleaseAssets();
}

void eae6320::OneShot::cTimingLight::SubmitToBeRendered( const bool i_canShoot )
{
	m_frame.SubmitToBeRendered( m_transform_localToWorld );
	if ( i_canShoot )
	{
		m_greenLamp.SubmitToBeRendered( m_transform_localToWorld );
		m_label.SubmitToBeRendered( m_transform_localToWorld );
	}
	else
	{
		m_redLamp.SubmitToBeRendered( m_transform_localToWorld );
	}
}
