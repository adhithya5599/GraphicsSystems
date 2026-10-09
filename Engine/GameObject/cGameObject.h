/*
	A game object is something in the world that can be drawn:
		* Its mesh, effect, and texture say how it is drawn
		* Its rigid body state says where it is and how it is moving

	The mesh, effect, and texture are reference counted assets
	that many game objects can share (e.g. every obstacle uses the same obstacle mesh):
	the game loads each asset once,
	each game object that uses an asset adds its own reference to it,
	and each game object releases its references when it is destroyed.
*/

#ifndef EAE6320_GAMEOBJECT_CGAMEOBJECT_H
#define EAE6320_GAMEOBJECT_CGAMEOBJECT_H

// Includes
//=========

#include <Engine/Physics/sRigidBodyState.h>

// Forward Declarations
//=====================

namespace eae6320
{
	namespace Graphics
	{
		class cMesh;
		class cEffect;
	}

	namespace Math
	{
		class cMatrix_transformation;
	}

	namespace Texture
	{
		class cTexture;
	}
}

// Class Declaration
//==================

namespace eae6320
{
	namespace GameObject
	{
		class cGameObject
		{
			// Interface
			//==========

		public:

			// Rendering
			//----------

			// Each setter adds a reference to the new asset and releases the reference to the old one
			// (so the caller keeps its own reference and is still responsible for releasing it)
			void SetMesh( Graphics::cMesh* const i_mesh );
			void SetEffect( Graphics::cEffect* const i_effect );
			void SetTexture( Texture::cTexture* const i_texture );

			// Submits this object to be drawn in the frame that is currently being submitted.
			// The simulation updates at a fixed rate that is usually slower than the frame rate,
			// so the position is extrapolated by the time since the last simulation update
			// to make movement look smooth.
			void SubmitToBeRendered( const float i_elapsedSecondCount_sinceLastSimulationUpdate );
			// Submits this object with an explicit transform
			// (for objects whose position doesn't come from their rigid body state, like a skybox that follows the camera)
			void SubmitToBeRendered( const Math::cMatrix_transformation& i_transform_localToWorld );

			// Movement
			//---------

			Physics::sRigidBodyState& GetRigidBodyState() { return m_rigidBodyState; }
			const Physics::sRigidBodyState& GetRigidBodyState() const { return m_rigidBodyState; }

			// Initialize / Clean Up
			//----------------------

			cGameObject() = default;
			~cGameObject();

			// Releases this object's references to its mesh, effect, and texture.
			// The destructor does this too, but a game must call this from its CleanUp():
			// the game itself is destroyed after the Graphics system has shut down,
			// and the last reference to a mesh/effect/texture must be released while Graphics still exists
			// (otherwise the GPU resources would be freed after the graphics context is gone).
			void ReleaseAssets();

			// A game object can be moved (e.g. when a std::vector of them grows)
			// but not copied, because a copy would release the same asset references a second time
			cGameObject( cGameObject&& io_gameObject ) noexcept;
			cGameObject& operator =( cGameObject&& io_gameObject ) noexcept;
			cGameObject( const cGameObject& ) = delete;
			cGameObject& operator =( const cGameObject& ) = delete;

			// Data
			//=====

		private:

			// These must start as nullptr because the destructor releases any that aren't
			Graphics::cMesh* m_mesh = nullptr;
			Graphics::cEffect* m_effect = nullptr;
			Texture::cTexture* m_texture = nullptr;

			Physics::sRigidBodyState m_rigidBodyState;
		};
	}
}

#endif	// EAE6320_GAMEOBJECT_CGAMEOBJECT_H
