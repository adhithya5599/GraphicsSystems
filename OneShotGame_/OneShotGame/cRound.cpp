// Includes
//=========

#include "cRound.h"

#include <algorithm>
#include <Engine/Asserts/Asserts.h>

// Interface
//==========

void eae6320::OneShot::cRound::Start()
{
	m_points = {};
	m_launchedCount = 0;
	m_scoredCount = 0;
	m_total = 0;
}

void eae6320::OneShot::cRound::LaunchArrow()
{
	EAE6320_ASSERTF( m_launchedCount < Settings::arrowsPerRound, "Every arrow in the round has already been launched" );
	EAE6320_ASSERTF( m_launchedCount == m_scoredCount, "An arrow was launched before the last one landed" );
	if ( m_launchedCount < Settings::arrowsPerRound )
	{
		++m_launchedCount;
	}
}

void eae6320::OneShot::cRound::RecordScore( const unsigned int i_points )
{
	EAE6320_ASSERTF( m_scoredCount < m_launchedCount, "A score was recorded for an arrow that wasn't launched" );
	if ( m_scoredCount >= m_launchedCount )
	{
		return;
	}
	m_points[m_scoredCount] = i_points;
	++m_scoredCount;
	m_total += i_points;
	if ( IsOver() )
	{
		m_bestTotal = m_hasBestTotal ? std::max( m_bestTotal, m_total ) : m_total;
		m_hasBestTotal = true;
	}
}

unsigned int eae6320::OneShot::cRound::GetPoints( const unsigned int i_arrowIndex ) const
{
	EAE6320_ASSERT( i_arrowIndex < m_scoredCount );
	return ( i_arrowIndex < m_scoredCount ) ? m_points[i_arrowIndex] : 0;
}
