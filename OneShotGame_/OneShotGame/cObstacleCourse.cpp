// Includes
//=========

#include "cObstacleCourse.h"

#include "cArrow.h"
#include "GameplaySettings.h"
#include "sGameAssets.h"

#include <algorithm>
#include <cmath>

// Helper Declarations
//====================

namespace
{
	constexpr float s_twoPi = 6.28318531f;

	void SetUpGameObject( eae6320::GameObject::cGameObject& io_gameObject, eae6320::Graphics::cMesh* const i_mesh,
		const eae6320::OneShot::sGameAssets& i_assets, eae6320::Texture::cTexture* const i_texture, const eae6320::Math::sVector& i_position );
}

// Interface
//==========

// Initialize / Clean Up
//----------------------

void eae6320::OneShot::cObstacleCourse::Initialize( const sGameAssets& i_assets )
{
	namespace S = Settings;
	m_movingParts.clear();
	m_frames.clear();
	m_frameColliders.clear();
	m_ropes.clear();
	// Every moving part is added before anything keeps a reference into the vector
	m_movingParts.reserve( 5 );

	// The rail slider
	{
		const Math::sVector position( 0.0f, 0.0f, S::RailSlider::z );
		m_frames.emplace_back();
		SetUpGameObject( m_frames.back(), i_assets.railSliderFrameMesh, i_assets, i_assets.whiteTexture, position );
		// The rail across the top (the posts are outside of the area the arrow can reach)
		m_frameColliders.push_back( Physics::sAabb3D{ position + Math::sVector( 0.0f, 3.2f, 0.0f ), Math::sVector( 7.2f, 0.075f, 0.06f ) } );

		sMovingPart plank;
		SetUpGameObject( plank.gameObject, i_assets.railSliderPlankMesh, i_assets, i_assets.woodTexture, position );
		plank.collider_local = Physics::sAabb3D{ S::RailSlider::plankCenter_local, S::RailSlider::plankHalfExtents };
		plank.basePosition = position;
		plank.motion = eMotion::SwingX;
		plank.amplitude = S::RailSlider::amplitude;
		plank.periodInSeconds = S::RailSlider::periodInSeconds;
		plank.phase = S::RailSlider::phase;
		m_movingParts.push_back( std::move( plank ) );
	}
	// The bunker
	{
		const Math::sVector position( 0.0f, 0.0f, S::Bunker::z );
		m_frames.emplace_back();
		SetUpGameObject( m_frames.back(), i_assets.bunkerMesh, i_assets, i_assets.whiteTexture, position );
		m_frameColliders.push_back( Physics::sAabb3D{ position + S::Bunker::wallCenter_local, S::Bunker::wallHalfExtents } );

		for ( size_t i = 0; i < S::Bunker::boardXs.size(); ++i )
		{
			const Math::sVector boardPosition( S::Bunker::boardXs[i], S::Bunker::boardHiddenHeight, S::Bunker::boardZ );
			sMovingPart board;
			SetUpGameObject( board.gameObject, i_assets.popupBoardMesh, i_assets, i_assets.woodTexture, boardPosition );
			board.collider_local = Physics::sAabb3D{ S::Bunker::boardCenter_local, S::Bunker::boardHalfExtents };
			board.basePosition = boardPosition;
			board.motion = eMotion::PopUp;
			board.triggerDistance = S::Bunker::boardTriggerDistances[i];
			board.riseHeight = S::Bunker::boardRaisedHeight - S::Bunker::boardHiddenHeight;
			board.riseDurationInSeconds = S::Bunker::boardRiseDurationInSeconds;
			m_movingParts.push_back( std::move( board ) );
		}
	}
	// The pulley
	{
		const Math::sVector position( 0.0f, 0.0f, S::Pulley::z );
		m_frames.emplace_back();
		SetUpGameObject( m_frames.back(), i_assets.pulleyFrameMesh, i_assets, i_assets.whiteTexture, position );
		for ( const auto postX : S::Pulley::postXs )
		{
			m_frameColliders.push_back( Physics::sAabb3D{ position + Math::sVector( postX, S::Pulley::postHalfExtents.y, 0.0f ), S::Pulley::postHalfExtents } );
		}

		sMovingPart board;
		SetUpGameObject( board.gameObject, i_assets.pulleyBoardMesh, i_assets, i_assets.woodTexture, position );
		board.collider_local = Physics::sAabb3D{ S::Pulley::boardCenter_local, S::Pulley::boardHalfExtents };
		board.basePosition = position + Math::sVector( 0.0f, S::Pulley::middleHeight, 0.0f );
		board.motion = eMotion::SwingY;
		board.amplitude = S::Pulley::amplitude;
		board.periodInSeconds = S::Pulley::periodInSeconds;
		board.phase = S::Pulley::phase;
		m_pulleyBoardIndex = m_movingParts.size();
		m_movingParts.push_back( std::move( board ) );

		for ( const auto ropeX : S::Pulley::ropeXs )
		{
			m_ropes.emplace_back();
			SetUpGameObject( m_ropes.back(), i_assets.ropeMesh, i_assets, i_assets.whiteTexture,
				position + Math::sVector( ropeX, S::Pulley::ropeTopHeight, 0.0f ) );
		}
	}
	// The sled
	{
		const Math::sVector position( 0.0f, 0.0f, S::Sled::z );
		// (The tracks are only a few centimeters tall, lower than the arrow can fly, so they don't need a collider)
		m_frames.emplace_back();
		SetUpGameObject( m_frames.back(), i_assets.sledTrackMesh, i_assets, i_assets.whiteTexture, position );

		sMovingPart plank;
		SetUpGameObject( plank.gameObject, i_assets.sledPlankMesh, i_assets, i_assets.woodTexture, position );
		plank.collider_local = Physics::sAabb3D{ S::Sled::plankCenter_local, S::Sled::plankHalfExtents };
		plank.basePosition = position;
		plank.motion = eMotion::SwingX;
		plank.amplitude = S::Sled::amplitude;
		plank.periodInSeconds = S::Sled::periodInSeconds;
		plank.phase = S::Sled::phase;
		m_movingParts.push_back( std::move( plank ) );
	}

	m_elapsedSecondCount = 0.0f;
	Reset();
	MoveParts( 0.0f, Settings::arrowRestPosition, false );
}

void eae6320::OneShot::cObstacleCourse::ReleaseAssets()
{
	for ( auto& part : m_movingParts )
	{
		part.gameObject.ReleaseAssets();
	}
	for ( auto* const gameObjects : { &m_frames, &m_ropes } )
	{
		for ( auto& gameObject : *gameObjects )
		{
			gameObject.ReleaseAssets();
		}
	}
}

void eae6320::OneShot::cObstacleCourse::Reset()
{
	for ( auto& part : m_movingParts )
	{
		if ( part.motion == eMotion::PopUp )
		{
			part.isRising = false;
			part.riseProgress = 0.0f;
		}
	}
}

// Update
//-------

void eae6320::OneShot::cObstacleCourse::Update( const float i_elapsedSecondCount_sinceLastUpdate, const Math::sVector& i_arrowTipPosition,
	const bool i_isArrowFlying )
{
	m_elapsedSecondCount += i_elapsedSecondCount_sinceLastUpdate;
	MoveParts( i_elapsedSecondCount_sinceLastUpdate, i_arrowTipPosition, i_isArrowFlying );
}

bool eae6320::OneShot::cObstacleCourse::IsHitBy( const cArrow& i_arrow, float& o_timeOfImpact ) const
{
	const auto tipAtStart = i_arrow.GetTipAtStartOfLastUpdate();
	const auto tipMovement = i_arrow.GetTipMovementDuringLastUpdate();
	const auto shaft = i_arrow.GetShaftCollider();
	auto isHit = false;
	o_timeOfImpact = 1.0f;
	const auto testCollider = [&]( const Physics::sAabb3D& i_collider )
	{
		// The tip is swept along its path so that it can't skip through something between updates
		// (and if it touches more than one thing, the earliest touch is the one that counts)
		float timeOfImpact;
		if ( Physics::Collision3D::SweepSphereAgainstAabb( tipAtStart, tipMovement, i_collider, timeOfImpact ) )
		{
			isHit = true;
			o_timeOfImpact = std::min( o_timeOfImpact, timeOfImpact );
		}
		// The shaft is checked where it is now so that clipping something while steering also counts
		// (there's no sweep for the shaft, so this counts as touching at the end of the update)
		else if ( Physics::Collision3D::DoAabbsOverlap( shaft, i_collider ) )
		{
			isHit = true;
		}
	};
	for ( const auto& part : m_movingParts )
	{
		testCollider( Physics::sAabb3D{ part.gameObject.GetRigidBodyState().position + part.collider_local.center, part.collider_local.halfExtents } );
	}
	for ( const auto& collider : m_frameColliders )
	{
		testCollider( collider );
	}
	return isHit;
}

// Render
//-------

void eae6320::OneShot::cObstacleCourse::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	StretchRopes( i_elapsedSecondCount_sinceLastSimulationUpdate );
	for ( auto& part : m_movingParts )
	{
		part.gameObject.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
	}
	for ( auto* const gameObjects : { &m_frames, &m_ropes } )
	{
		for ( auto& gameObject : *gameObjects )
		{
			gameObject.SubmitToBeRendered( i_elapsedSecondCount_sinceLastSimulationUpdate );
		}
	}
}

// Implementation
//===============

void eae6320::OneShot::cObstacleCourse::MoveParts( const float i_elapsedSecondCount_sinceLastUpdate, const Math::sVector& i_arrowTipPosition,
	const bool i_isArrowFlying )
{
	for ( auto& part : m_movingParts )
	{
		auto& rigidBodyState = part.gameObject.GetRigidBodyState();
		switch ( part.motion )
		{
		case eMotion::SwingX:
		case eMotion::SwingY:
			{
				// The position is calculated directly from the time (rather than by adding up velocities)
				// so a part can never drift away from its path,
				// but the velocity (the derivative of the position) is also set
				// because the renderer uses it to draw the part smoothly between simulation updates
				const auto angle = s_twoPi * ( ( m_elapsedSecondCount / part.periodInSeconds ) + part.phase );
				const auto offset = part.amplitude * std::sin( angle );
				const auto speed = part.amplitude * ( s_twoPi / part.periodInSeconds ) * std::cos( angle );
				const auto direction = ( part.motion == eMotion::SwingX ) ? Math::sVector( 1.0f, 0.0f, 0.0f ) : Math::sVector( 0.0f, 1.0f, 0.0f );
				rigidBodyState.position = part.basePosition + ( direction * offset );
				rigidBodyState.velocity = direction * speed;
			}
			break;
		case eMotion::PopUp:
			{
				// A simple trigger, not decision making: once the arrow's tip is close enough in front of the board, it rises
				const auto distanceInFront = i_arrowTipPosition.z - part.basePosition.z;
				if ( i_isArrowFlying && !part.isRising && ( distanceInFront > 0.0f ) && ( distanceInFront < part.triggerDistance ) )
				{
					part.isRising = true;
				}
				if ( part.isRising && ( part.riseProgress < 1.0f ) )
				{
					part.riseProgress = std::min( part.riseProgress + ( i_elapsedSecondCount_sinceLastUpdate / part.riseDurationInSeconds ), 1.0f );
				}
				rigidBodyState.position = part.basePosition + Math::sVector( 0.0f, part.riseHeight * part.riseProgress, 0.0f );
				const auto isMoving = part.isRising && ( part.riseProgress < 1.0f );
				rigidBodyState.velocity = Math::sVector( 0.0f, isMoving ? ( part.riseHeight / part.riseDurationInSeconds ) : 0.0f, 0.0f );
			}
			break;
		}
	}
}

void eae6320::OneShot::cObstacleCourse::StretchRopes( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	// The rope mesh is 1 m long hanging down from its origin, so stretching it vertically sets its length.
	// The board's top is predicted the same way the renderer predicts the board's position,
	// so the ropes and the board stay attached even between simulation updates.
	const auto& board = m_movingParts[m_pulleyBoardIndex].gameObject.GetRigidBodyState();
	const auto boardTop = board.PredictFuturePosition( i_elapsedSecondCount_sinceLastSimulationUpdate ).y;
	for ( auto& rope : m_ropes )
	{
		const auto length = std::max( rope.GetRigidBodyState().position.y - boardTop, 0.01f );
		rope.SetScale( Math::sVector( 1.0f, length, 1.0f ) );
	}
}

// Helper Definitions
//===================

namespace
{
	void SetUpGameObject( eae6320::GameObject::cGameObject& io_gameObject, eae6320::Graphics::cMesh* const i_mesh,
		const eae6320::OneShot::sGameAssets& i_assets, eae6320::Texture::cTexture* const i_texture, const eae6320::Math::sVector& i_position )
	{
		io_gameObject.SetMesh( i_mesh );
		io_gameObject.SetEffect( i_assets.litEffect );
		io_gameObject.SetTexture( i_texture );
		io_gameObject.GetRigidBodyState().position = i_position;
	}
}
