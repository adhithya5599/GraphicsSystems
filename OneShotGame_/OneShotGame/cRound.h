/*
	A round keeps score: six arrows (an Olympic "end"), the points each one scored, and the best total so far

	It only keeps the numbers; the big screen (cScoreboard) and the quiver (cQuiver) show them.
	Keeping the score separate from how it is shown means the rules can be checked without drawing anything,
	and the screen can't get out of step with the score because it reads the score every frame.
*/

#ifndef EAE6320_ONESHOT_CROUND_H
#define EAE6320_ONESHOT_CROUND_H

// Includes
//=========

#include "GameplaySettings.h"

#include <array>

// Class Declaration
//==================

namespace eae6320
{
	namespace OneShot
	{
		class cRound
		{
			// Interface
			//==========

		public:

			// Starts a new round with all of its arrows (the best total is kept)
			void Start();

			// An arrow leaves the bow
			void LaunchArrow();
			// The arrow that was launched landed and scored this many points (0 is a miss)
			void RecordScore( const unsigned int i_points );

			// Arrows that haven't been launched yet
			unsigned int GetArrowsLeft() const { return Settings::arrowsPerRound - m_launchedCount; }
			// How many arrows have a score so far (the arrow that is flying doesn't, yet)
			unsigned int GetScoredCount() const { return m_scoredCount; }
			// The points that an arrow scored (only valid for i_arrowIndex < GetScoredCount())
			unsigned int GetPoints( const unsigned int i_arrowIndex ) const;
			unsigned int GetTotal() const { return m_total; }
			// The round is over once every arrow has a score
			bool IsOver() const { return m_scoredCount >= Settings::arrowsPerRound; }

			// The best total of any round that has been finished (there is none until the first round is over)
			bool HasBestTotal() const { return m_hasBestTotal; }
			unsigned int GetBestTotal() const { return m_bestTotal; }

			// Data
			//=====

		private:

			std::array<unsigned int, Settings::arrowsPerRound> m_points = {};
			unsigned int m_launchedCount = 0;
			unsigned int m_scoredCount = 0;
			unsigned int m_total = 0;
			unsigned int m_bestTotal = 0;
			bool m_hasBestTotal = false;
		};
	}
}

#endif	// EAE6320_ONESHOT_CROUND_H
