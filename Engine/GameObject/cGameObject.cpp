// Includes
//=========

#include "cGameObject.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Graphics/cEffect.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/Graphics.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Texture/cTexture.h>

#include <utility>

// Helper Declarations
//====================

namespace
{
	// Replaces the asset that io_current points to with i_new, keeping the reference counts correct
	template <class tAsset>
	void ReplaceReference( tAsset*& io_current, tAsset* const i_new );
}

// Interface
//==========

// Rendering
//----------

void eae6320::GameObject::cGameObject::SetMesh( Graphics::cMesh* const i_mesh )
{
	ReplaceReference( m_mesh, i_mesh );
}

void eae6320::GameObject::cGameObject::SetEffect( Graphics::cEffect* const i_effect )
{
	ReplaceReference( m_effect, i_effect );
}

void eae6320::GameObject::cGameObject::SetTexture( Texture::cTexture* const i_texture )
{
	ReplaceReference( m_texture, i_texture );
}

void eae6320::GameObject::cGameObject::SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate )
{
	SubmitToBeRendered( m_rigidBodyState.PredictFutureTransform( i_elapsedSecondCount_sinceLastSimulationUpdate ) );
}

void eae6320::GameObject::cGameObject::SubmitToBeRendered( const Math::cMatrix_transformation& i_transform_localToWorld )
{
	// The Graphics system can't draw something without geometry and shaders
	// (a missing texture is allowed, but the standard shaders will sample black from it)
	EAE6320_ASSERTF( m_mesh && m_effect, "A game object must have a mesh and an effect before it can be rendered" );
	if ( m_mesh && m_effect )
	{
		// The submission function takes a non-const reference, so it needs a copy that it is allowed to modify
		auto transform_localToWorld = i_transform_localToWorld;
		constexpr unsigned int textureSlot = 0;
		Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame( m_mesh, m_effect, transform_localToWorld, m_texture, textureSlot );
	}
}

// Initialize / Clean Up
//----------------------

eae6320::GameObject::cGameObject::~cGameObject()
{
	ReleaseAssets();
}

eae6320::GameObject::cGameObject::cGameObject( cGameObject&& io_gameObject ) noexcept
	:
	// Moving takes over the other object's references instead of adding new ones,
	// and then the other object forgets them so that its destructor doesn't release them
	m_mesh( std::exchange( io_gameObject.m_mesh, nullptr ) ),
	m_effect( std::exchange( io_gameObject.m_effect, nullptr ) ),
	m_texture( std::exchange( io_gameObject.m_texture, nullptr ) ),
	m_rigidBodyState( io_gameObject.m_rigidBodyState )
{

}

eae6320::GameObject::cGameObject& eae6320::GameObject::cGameObject::operator =( cGameObject&& io_gameObject ) noexcept
{
	if ( this != &io_gameObject )
	{
		ReleaseAssets();
		m_mesh = std::exchange( io_gameObject.m_mesh, nullptr );
		m_effect = std::exchange( io_gameObject.m_effect, nullptr );
		m_texture = std::exchange( io_gameObject.m_texture, nullptr );
		m_rigidBodyState = io_gameObject.m_rigidBodyState;
	}
	return *this;
}

void eae6320::GameObject::cGameObject::ReleaseAssets()
{
	ReplaceReference<Graphics::cMesh>( m_mesh, nullptr );
	ReplaceReference<Graphics::cEffect>( m_effect, nullptr );
	ReplaceReference<Texture::cTexture>( m_texture, nullptr );
}

// Helper Definitions
//===================

namespace
{
	template <class tAsset>
	void ReplaceReference( tAsset*& io_current, tAsset* const i_new )
	{
		// The new reference is added before the old one is released
		// so that replacing an asset with itself can't delete it by accident
		if ( i_new )
		{
			i_new->IncrementReferenceCount();
		}
		if ( io_current )
		{
			io_current->DecrementReferenceCount();
		}
		io_current = i_new;
	}
}
