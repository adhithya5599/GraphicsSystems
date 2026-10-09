#pragma once
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector2.h>
#include <Engine/Results/cResult.h>
#include <Engine/Physics/AABB.h>
#include <Engine/Graphics/cMesh.h>
#include <Engine/Graphics/cEffect.h>
#include <Engine/Math/cMatrix_transformation.h>

#include <memory>
#include <string>
#include <vector>

namespace eae6320
{
	enum EShapeType
	{
		Circle = 0,
		Box = 1,
		Polygon = 2,
	};
	namespace Physics
	{
		class cPhysicsWorld;
		class PhysicsBody2D
		{
		public:
			
			virtual ~PhysicsBody2D();
			cResult Initialize();
			cResult CleanUp();

			// Interface
			//==========
			
			virtual void Update(const float& i_secondCountToIntegrate, const float i_elapsedSecondCount_sinceLastSimulationUpdate);
			const Math::sVector2 PredictFuturePosition(const float i_secondCountToExtrapolate) const;
			const Math::cQuaternion PredictFutureOrientation(const float i_secondCountToExtrapolate) const;
			const Math::cMatrix_transformation PredictFutureTransform(const float i_secondCountToExtrapolate) const;
			void Move(const Math::sVector2& i_dir);
			void MoveTo(const Math::sVector2& i_pos);
			void Rotate(float i_angle);
			void RotateTo(float i_angle);
			void AddForce(const Math::sVector2& i_amount);
			void Transform(const Math::cMatrix_transformation & i_transform);
			Physics::AABB GetAABB();

			std::vector<Math::sVector> GetTransformedVertices();
			std::vector<Math::sVector2> GetTransformedVertices2D();
			Math::cMatrix_transformation GetTransformedMatrix();

			Math::sVector2 position;	// In arbitrary units determined by the applicaton's convention
			Math::sVector2 velocity;	// Distance per second
			Math::sVector2 acceleration;	// Distance per second^2
			Math::cMatrix_transformation transform;
			float angle = 0;
			float angularVelocity = 0;	// Radians per second (positive values rotate right-handed, negative rotate left-handed)
			Math::sVector2 Force;

			const EShapeType ShapeType;
			const float Density;
			const float Mass;
			// 很多计算用到mass的倒数：1/Mass，所以预计算倒数
			const float InvMass;
			const float Restitution;
			const float Area;
			const bool bIsStatic;
			const float Radius;
			const float Width;
			const float Height;
			float Inertia;
			float InvInertia;
			float StaticFriction;
			float DynamicFriction;

			std::vector<Math::sVector> m_vertices;
			std::vector<Math::sVector> m_transformedVertices;
			std::vector<Math::sVector2> m_transformedVertices2D;
			AABB m_aabb;

			bool bTransformUpdateRequired;
			bool aabbUpdateRequired;


			virtual bool ShouldRenderDebug() const { return m_bEnableDebugDraw; }

			void SetDebugDrawEnabled(bool enabled) { m_bEnableDebugDraw = enabled; }

			friend std::unique_ptr<PhysicsBody2D> CreateBoxBody(float i_width, float i_height, float i_density, bool i_isStatic, float i_restitution, std::string& errorMessage);
			friend std::unique_ptr<PhysicsBody2D> CreateBoxBody(float i_width, float i_height, float i_density, bool i_isStatic, float i_restitution);
			friend std::unique_ptr<PhysicsBody2D> CreateCircleBody(float i_radius, float i_density, bool i_isStatic, float i_restitution, std::string& errorMessage);
			friend std::unique_ptr<PhysicsBody2D> CreateCircleBody(float i_radius, float i_density, bool i_isStatic, float i_restitution);
			

		private:
			PhysicsBody2D(
				float i_density, float i_mass, float i_inertia, float i_restitution, const float i_area,
				bool i_isStatic, float i_radius, float i_width, float i_height, const std::vector<Math::sVector>& i_vertices, EShapeType i_shapeType);
				
			virtual void Step(float i_secondCountToIntegrate, const Math::sVector2& i_gravity, const int i_iterations);
			float CalculateRotationalInertia();

			friend class eae6320::Physics::cPhysicsWorld;

			bool m_bEnableDebugDraw = true;
			
			eae6320::Graphics::cMesh* m_debugMesh = nullptr;
			eae6320::Graphics::cEffect* m_debugEffect = nullptr;
		};
		std::unique_ptr<PhysicsBody2D> CreateBoxBody(float i_width, float i_height, float i_density, bool i_isStatic, float i_restitution, std::string& errorMessage);
		std::unique_ptr<PhysicsBody2D> CreateBoxBody(float i_width, float i_height, float i_density, bool i_isStatic, float i_restitution);
		std::unique_ptr<PhysicsBody2D> CreateCircleBody(float i_radius, float i_density, bool i_isStatic, float i_restitution, std::string& errorMessage);
		std::unique_ptr<PhysicsBody2D> CreateCircleBody(float i_radius, float i_density, bool i_isStatic, float i_restitution);
	
		inline void ConstructBox(float i_width, float i_height, std::vector<eae6320::Math::sVector>& o_vertices)
		{
			float halfWidth = i_width * 0.5f;
			float halfHeight = i_height * 0.5f;

			o_vertices.resize(4);

			o_vertices[0] = { -halfWidth, -halfHeight, 0 };
			o_vertices[1] = { halfWidth, -halfHeight, 0 };
			o_vertices[2] = { halfWidth,  halfHeight, 0 };
			o_vertices[3] = { -halfWidth,  halfHeight, 0 };
		}
	}
}