// Includes
//=========

#include "cScoreboard.h"

#include "cRound.h"
#include "GameplaySettings.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <Engine/Math/cQuaternion.h>

// Static Data
//============

namespace
{
	// The character meshes' font: every character is 5 x 7 dots, and each character takes 6 dots of width (5 dots plus a gap).
	// A character mesh is 1 m tall (the height of its dots), so it is scaled by the height that it needs to be.
	// (These match scoreboard_layout.py, which made the meshes)
	constexpr float s_dotRows = 7.0f;
	constexpr float s_characterAdvanceInDots = 6.0f;
}

// Interface
//==========

void eae6320::OneShot::cScoreboard::Initialize( const sGameAssets& i_assets )
{
	{
		const auto yaw = Settings::Scoreboard::yawDegrees * 3.14159265f / 180.0f;
		m_transform_localToWorld = Math::cMatrix_transformation( Math::cQuaternion( yaw, Math::sVector( 0.0f, 1.0f, 0.0f ) ),
			Settings::Scoreboard::position );
	}
	m_frame.SetMesh( i_assets.scoreboardFrameMesh );
	m_frame.SetEffect( i_assets.litEffect );
	m_frame.SetTexture( i_assets.whiteTexture );
	// The display gives off its own light, so it is drawn unlit
	// (and black stays exactly black, so that the characters blend into it without visible edges)
	m_display.SetMesh( i_assets.scoreboardDisplayMesh );
	m_display.SetEffect( i_assets.unlitEffect );
	m_display.SetTexture( i_assets.scoreboardTexture );
	for ( size_t i = 0; i < m_glyphs.size(); ++i )
	{
		m_glyphs[i].SetMesh( i_assets.scoreGlyphMeshes[i] );
		m_glyphs[i].SetEffect( i_assets.unlitEffect );
		m_glyphs[i].SetTexture( i_assets.scoreboardTexture );
	}
}

void eae6320::OneShot::cScoreboard::ReleaseAssets()
{
	m_frame.ReleaseAssets();
	m_display.ReleaseAssets();
	for ( auto& glyph : m_glyphs )
	{
		glyph.ReleaseAssets();
	}
}

void eae6320::OneShot::cScoreboard::SubmitToBeRendered( const cRound& i_round, const float i_elapsedSecondCount_systemTime )
{
	m_frame.SubmitToBeRendered( m_transform_localToWorld );
	m_display.SubmitToBeRendered( m_transform_localToWorld );

	char text[16];
	// The round's total
	std::snprintf( text, sizeof( text ), "%u", i_round.GetTotal() );
	SubmitTextRightAligned( text, Settings::Scoreboard::totalRight, Settings::Scoreboard::totalBottom, Settings::Scoreboard::totalHeight );
	// The best total so far (dashes until a round has been finished)
	if ( i_round.HasBestTotal() )
	{
		std::snprintf( text, sizeof( text ), "%u", i_round.GetBestTotal() );
	}
	else
	{
		std::snprintf( text, sizeof( text ), "--" );
	}
	SubmitTextRightAligned( text, Settings::Scoreboard::bestRight, Settings::Scoreboard::bestBottom, Settings::Scoreboard::bestHeight );
	// How many arrows are left
	std::snprintf( text, sizeof( text ), "%u", i_round.GetArrowsLeft() );
	SubmitText( text, Settings::Scoreboard::arrowsLeftLeft, Settings::Scoreboard::arrowsLeftBottom, Settings::Scoreboard::arrowsLeftHeight );
	// Each arrow's score in its slot ("M" is what archery scoreboards show for an arrow that scored nothing).
	// A slot whose arrow hasn't landed yet is empty, so the empty slots show how many arrows are still to come.
	const auto newestArrowIndex = static_cast<int>( i_round.GetScoredCount() ) - 1;
	const auto isNewestScoreHidden = m_isNewestScoreFlashing
		&& ( std::fmod( i_elapsedSecondCount_systemTime, Settings::Scoreboard::flashPeriodInSeconds )
			> ( Settings::Scoreboard::flashOnFraction * Settings::Scoreboard::flashPeriodInSeconds ) );
	for ( unsigned int arrowIndex = 0; arrowIndex < i_round.GetScoredCount(); ++arrowIndex )
	{
		if ( isNewestScoreHidden && ( static_cast<int>( arrowIndex ) == newestArrowIndex ) )
		{
			continue;
		}
		const auto points = i_round.GetPoints( arrowIndex );
		if ( points > 0 )
		{
			std::snprintf( text, sizeof( text ), "%u", points );
		}
		else
		{
			std::snprintf( text, sizeof( text ), "M" );
		}
		// The slots are spread evenly across the screen, centered on its middle
		const auto slotCenter = ( static_cast<float>( arrowIndex ) - ( static_cast<float>( Settings::arrowsPerRound - 1 ) * 0.5f ) )
			* Settings::Scoreboard::slotSpacing;
		SubmitTextCentered( text, slotCenter, Settings::Scoreboard::slotBottom, Settings::Scoreboard::slotHeight );
	}
}

// Implementation
//===============

void eae6320::OneShot::cScoreboard::SubmitText( const char* const i_text, const float i_left, const float i_bottom, const float i_height )
{
	const auto advance = i_height * ( s_characterAdvanceInDots / s_dotRows );
	const auto scale = Math::cMatrix_transformation::CreateScale( i_height, i_height, i_height );
	for ( size_t i = 0; i_text[i] != '\0'; ++i )
	{
		const auto glyphIndex = GetGlyphIndex( i_text[i] );
		if ( glyphIndex < 0 )
		{
			continue;
		}
		// Each character mesh has the bottom-left of its dots at its origin and faces +z, like the screen,
		// so it is scaled, moved to its place on the screen (in front of the display), and then moved with the screen
		const Math::sVector position_local( i_left + ( static_cast<float>( i ) * advance ),
			Settings::Scoreboard::displayCenterHeight + i_bottom, Settings::Scoreboard::numbersZ );
		const auto transform_characterToScreen = Math::cMatrix_transformation::ConcatenateAffine(
			Math::cMatrix_transformation( Math::cQuaternion(), position_local ), scale );
		m_glyphs[static_cast<size_t>( glyphIndex )].SubmitToBeRendered(
			Math::cMatrix_transformation::ConcatenateAffine( m_transform_localToWorld, transform_characterToScreen ) );
	}
}

void eae6320::OneShot::cScoreboard::SubmitTextRightAligned( const char* const i_text, const float i_right, const float i_bottom, const float i_height )
{
	// The width of a line of text's dots: 6 dots for each character except the last, which is only 5 (it has no gap after it)
	const auto characterCount = static_cast<float>( std::strlen( i_text ) );
	const auto width = ( ( characterCount * s_characterAdvanceInDots ) - 1.0f ) * ( i_height / s_dotRows );
	SubmitText( i_text, i_right - width, i_bottom, i_height );
}

void eae6320::OneShot::cScoreboard::SubmitTextCentered( const char* const i_text, const float i_center, const float i_bottom, const float i_height )
{
	const auto characterCount = static_cast<float>( std::strlen( i_text ) );
	const auto width = ( ( characterCount * s_characterAdvanceInDots ) - 1.0f ) * ( i_height / s_dotRows );
	SubmitText( i_text, i_center - ( width * 0.5f ), i_bottom, i_height );
}

int eae6320::OneShot::cScoreboard::GetGlyphIndex( const char i_character )
{
	// The character meshes are in this order in sGameAssets::scoreGlyphMeshes
	if ( ( i_character >= '0' ) && ( i_character <= '9' ) )
	{
		return i_character - '0';
	}
	switch ( i_character )
	{
	case 'M': return 10;
	case '-': return 11;
	default: return -1;
	}
}
