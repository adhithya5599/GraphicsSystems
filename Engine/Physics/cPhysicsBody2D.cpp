#include "cPhysicsBody2D.h"
#include <Engine/Math/Constants.h>
#include <Engine/GameObject/cGameObject.h>
#include <MyGame_/MyGame/cMyGame.h>
#include "cPhysicsWorld.h"
#include "PhysicsUtil.h"
#include <Engine/Logging/Logging.h>

eae6320::Physics::PhysicsBody2D::PhysicsBody2D(
	 float i_density, float i_mass, float i_inertia, float i_restitution, float i_area,
	bool i_isStatic, float i_radius, float i_width, float i_height, const std::vector<Math::sVector>& i_vertices, EShapeType i_shapeType)
	:Density(i_density), Mass(i_mass), Inertia(i_inertia), Restitution(i_restitution), Area(i_area), bIsStatic(i_isStatic),
	Radius(i_radius), Width(i_width), Height(i_height), ShapeType(i_shapeType),
	InvMass(i_mass > 0.f ? (i_isStatic ? 0.f : 1.f / i_mass) : 0),// 使用值为0的Inverse Mass来代表静态物体
	m_aabb(0, 0, 0, 0)
{
	InvInertia = Inertia > 0.f ? (bIsStatic? 0.f : 1.f / Inertia) : 0.f;
	angle = 0.f;
	angularVelocity = 0.f;
	StaticFriction = 0.6f;
	DynamicFriction = 0.4f;
	Force = Math::sVector2::Zero();

	if (ShapeType == EShapeType::Box)
	{
		for (unsigned int i = 0; i < i_vertices.size(); i++)
		{
			m_vertices.emplace_back(i_vertices[i]);
		}
		ConstructBox(i_width, i_height, m_vertices);
		m_transformedVertices.resize(4);
		m_transformedVertices2D.resize(4);
	}
	else
	{
		m_vertices.resize(0);
		m_transformedVertices.resize(0);
		m_transformedVertices2D.resize(0);
	}

	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();
}
eae6320::Physics::PhysicsBody2D::~PhysicsBody2D()
{
}

eae6320::cResult eae6320::Physics::PhysicsBody2D::Initialize()
{
	return cResult();
}

eae6320::cResult eae6320::Physics::PhysicsBody2D::CleanUp()
{
	return cResult();
}

void eae6320::Physics::PhysicsBody2D::Step(float i_deltatime, const Math::sVector2& i_gravity, const int i_iterations)
{
	if (std::abs(Mass) > 0.0001f)
	{
		Math::sVector2 acceleration = Force / Mass;
		velocity += acceleration * i_deltatime;
	}

	if (bIsStatic) return;

	i_deltatime /= (float)i_iterations;

	velocity += i_gravity * i_deltatime;
	if(velocity.GetLength() < 0.001f)
	{
		velocity = Math::sVector2::Zero();
	}

	position += velocity * i_deltatime;

	if (std::abs(angularVelocity) < 0.001f)
	{
		angularVelocity = 0;
	}
	angle += angularVelocity * i_deltatime;
	
	
	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();

	Force = Math::sVector2::Zero();
}

float eae6320::Physics::PhysicsBody2D::CalculateRotationalInertia()
{
	if (ShapeType == EShapeType::Circle)
	{
		return (1.f / 2.f)* Mass* Radius* Radius;
	}
	else if (ShapeType == EShapeType::Box)
	{
		return (1.f / 12.f) * Mass * (Width * Width + Height * Height);
	}
	else
	{
		return 0.f;
		Logging::OutputError("Shape type for calculating rotational inertia is not a valid shape type");
	}
}

void eae6320::Physics::PhysicsBody2D::Update(const float& i_secondCountToIntegrate, const float i_elapsedSecondCount_sinceLastSimulationUpdate)
{
	// Update position
	{
		position += velocity * i_secondCountToIntegrate;
	}
	// Update velocity
	{
		velocity += acceleration * i_secondCountToIntegrate;
	}
	// Update orientation
	{
		angle += angularVelocity * i_secondCountToIntegrate;
	}

}

const eae6320::Math::sVector2 eae6320::Physics::PhysicsBody2D::PredictFuturePosition(const float i_secondCountToExtrapolate) const
{
	return position + (velocity * i_secondCountToExtrapolate);
}

const eae6320::Math::cQuaternion eae6320::Physics::PhysicsBody2D::PredictFutureOrientation(const float i_secondCountToExtrapolate) const
{
	return Math::cQuaternion();
}

const eae6320::Math::cMatrix_transformation eae6320::Physics::PhysicsBody2D::PredictFutureTransform(const float i_secondCountToExtrapolate) const
{
	return Util::CreateTransform2DTo3D(PredictFuturePosition(i_secondCountToExtrapolate), angle + angularVelocity * i_secondCountToExtrapolate);
}

void eae6320::Physics::PhysicsBody2D::Move(const eae6320::Math::sVector2& i_dir)
{
	position += i_dir;
	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();
}

void eae6320::Physics::PhysicsBody2D::MoveTo(const Math::sVector2& i_pos)
{
	position = i_pos;
	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();
}

void eae6320::Physics::PhysicsBody2D::Rotate(float i_angle)
{
	angle += i_angle;
	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();
}

void eae6320::Physics::PhysicsBody2D::RotateTo(float i_angle)
{
	angle = i_angle;
	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();
}

void eae6320::Physics::PhysicsBody2D::AddForce(const Math::sVector2& i_amount)
{
	Force = i_amount;
}



void eae6320::Physics::PhysicsBody2D::Transform(const Math::cMatrix_transformation& i_transform)
{
	Math::sVector vec(position.x, position.y, 0);
	vec = i_transform * vec;

	position.x = vec.x;
	position.y = vec.y;

	bTransformUpdateRequired = true;
	aabbUpdateRequired = true;
	GetTransformedVertices2D();
	GetAABB();
}

eae6320::Physics::AABB eae6320::Physics::PhysicsBody2D::GetAABB()
{
	if (aabbUpdateRequired)
	{
		float minX = FLT_MAX;
		float minY = FLT_MAX;
		float maxX = FLT_MIN;
		float maxY = FLT_MAX;
		if (ShapeType == EShapeType::Box)
		{
			auto vertices = GetTransformedVertices2D();
			for (unsigned int i = 0; i < vertices.size(); i++)
			{
				const Math::sVector2& v = vertices[i];
				if (v.x < minX) minX = v.x;
				if (v.y < minY) minY = v.y;
				if (v.x > maxX) maxX = v.x;
				if (v.y > maxY) maxY = v.y;
			}
		}
		else if (ShapeType == EShapeType::Circle)
		{
			minX = position.x - Radius;
			minY = position.y - Radius;
			maxX = position.x + Radius;
			maxY = position.y + Radius;
		}
		else
		{
			Logging::OutputError("Unkown ShapeType");
		}
		m_aabb = AABB(minX, minY, maxX, maxY);
	}

	aabbUpdateRequired = false;
	return m_aabb;
}

std::vector<eae6320::Math::sVector> eae6320::Physics::PhysicsBody2D::GetTransformedVertices()
{
	if (bTransformUpdateRequired)
	{
		for (unsigned int i = 0; i < m_vertices.size(); i++)
		{
			Math::sVector2 vec = Util::ToVec2(m_vertices[i]);
			Util::Transform2D(position, angle, vec);
			m_transformedVertices[i] = Util::ToVec3(vec);
		}

		transform = Util::CreateTransform2DTo3D(position, angle);
		bTransformUpdateRequired = false;
	}

	
	return m_transformedVertices;
}

std::vector<eae6320::Math::sVector2> eae6320::Physics::PhysicsBody2D::GetTransformedVertices2D()
{
	if (bTransformUpdateRequired)
	{
		for (unsigned int i = 0; i < m_vertices.size(); i++)
		{
			Math::sVector2 vec = Util::ToVec2(m_vertices[i]);
			Util::Transform2D(position, angle, vec);
			m_transformedVertices2D[i] = vec;
		}

		transform = Util::CreateTransform2DTo3D(position, angle);
		bTransformUpdateRequired = false;
	}

	
	return m_transformedVertices2D;
}

eae6320::Math::cMatrix_transformation eae6320::Physics::PhysicsBody2D::GetTransformedMatrix()
{
	if (bTransformUpdateRequired)
	{
		for (unsigned int i = 0; i < m_vertices.size(); i++)
		{
			Math::sVector2 vec = Util::ToVec2(m_vertices[i]);
			Util::Transform2D(position, angle, vec);
			m_transformedVertices[i] = Util::ToVec3(vec);
		}

		transform = Util::CreateTransform2DTo3D(position, angle);
		bTransformUpdateRequired = false;
	}

	
	return transform;
}


std::unique_ptr<eae6320::Physics::PhysicsBody2D> eae6320::Physics::CreateCircleBody(float i_radius, float i_density, bool i_isStatic, float i_restitution, std::string& errorMessage)
{
	std::unique_ptr<eae6320::Physics::PhysicsBody2D> body = nullptr;
	errorMessage = "";

	float area = i_radius * i_radius * Math::g_pi;

	if (area < Physics::cPhysicsWorld::MinBodySize)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "area is too small. Min area is %f", Physics::cPhysicsWorld::MinBodySize);
		return nullptr;
	}

	if (area > Physics::cPhysicsWorld::MaxBodySize)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "area is too large. Max area is %f", Physics::cPhysicsWorld::MaxBodySize);
		return nullptr;
	}

	if (i_density < Physics::cPhysicsWorld::MinDensity)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "density is too small. Min density is %f", Physics::cPhysicsWorld::MinDensity);
		return nullptr;
	}

	if (i_density > Physics::cPhysicsWorld::MaxDensity)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "density is too large. Max density is %f", Physics::cPhysicsWorld::MaxDensity);
		return nullptr;
	}

	i_restitution = Util::Clamp(i_restitution, 0.f, 1.f);

	float mass = 0.f;
	float inertia = 0.f;

	if (!i_isStatic)
	{
		mass = area * 1.f * i_density;
		inertia = (1.f / 2.f) * mass * i_radius * i_radius;
	}

	body = std::unique_ptr<Physics::PhysicsBody2D>(
		new Physics::PhysicsBody2D(
			i_density,
			mass, 
			inertia,
			i_restitution,
			area, i_isStatic, 
			i_radius, 
			0.f, 
			0.f, 
			std::vector<Math::sVector>(),
			EShapeType::Circle)
	);
	
	return body;
}

std::unique_ptr<eae6320::Physics::PhysicsBody2D> eae6320::Physics::CreateCircleBody(float i_radius, float i_density, bool i_isStatic, float i_restitution)
{
	std::unique_ptr<eae6320::Physics::PhysicsBody2D> body = nullptr;

	float area = i_radius * i_radius * Math::g_pi;

	if (area < Physics::cPhysicsWorld::MinBodySize)
	{
		return nullptr;
	}

	if (area > Physics::cPhysicsWorld::MaxBodySize)
	{
		return nullptr;
	}

	if (i_density < Physics::cPhysicsWorld::MinDensity)
	{
		return nullptr;
	}

	if (i_density > Physics::cPhysicsWorld::MaxDensity)
	{
		return nullptr;
	}

	i_restitution = Util::Clamp(i_restitution, 0.f, 1.f);

	float mass = 0.f;
	float inertia = 0.f;

	if (!i_isStatic)
	{
		mass = area * 1.f * i_density;
		inertia = (1.f / 2.f) * mass * i_radius * i_radius;
	}

	body = std::unique_ptr<Physics::PhysicsBody2D>(
		new Physics::PhysicsBody2D(
			i_density,
			mass,
			inertia,
			i_restitution,
			area, i_isStatic,
			i_radius,
			0.f,
			0.f,
			std::vector<Math::sVector>(),
			EShapeType::Circle)
	);

	return body;
}


std::unique_ptr<eae6320::Physics::PhysicsBody2D> eae6320::Physics::CreateBoxBody(float i_width, float i_height, float i_density, bool i_isStatic, float i_restitution, std::string& errorMessage)
{
	std::unique_ptr<eae6320::Physics::PhysicsBody2D> body = nullptr;
	errorMessage = "";

	float area = i_width * i_height;

	if (area < Physics::cPhysicsWorld::MinBodySize)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "area is too small. Min area is %f", Physics::cPhysicsWorld::MinBodySize);
		return nullptr;
	}

	if (area > Physics::cPhysicsWorld::MaxBodySize)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "area is too large. Max area is %f", Physics::cPhysicsWorld::MaxBodySize);
		return nullptr;
	}

	if (i_density < Physics::cPhysicsWorld::MinDensity)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "density is too small. Min density is %f", Physics::cPhysicsWorld::MinDensity);
		return nullptr;
	}

	if (i_density > Physics::cPhysicsWorld::MaxDensity)
	{
		snprintf(errorMessage.data(), errorMessage.size(), "density is too large. Max density is %f", Physics::cPhysicsWorld::MaxDensity);
		return nullptr;
	}

	i_restitution = Util::Clamp(i_restitution, 0.f, 1.f);

	float mass = 0.f;
	float inertia = 0.f;

	if (!i_isStatic)
	{
		mass = area * 1.f * i_density;
		inertia = (1.f / 12.f) * mass * (i_width * i_width + i_height * i_height);
	}

	std::vector<Math::sVector> vertices;
	ConstructBox(i_width, i_height, vertices);

	body = std::unique_ptr<Physics::PhysicsBody2D>(
		new Physics::PhysicsBody2D(
			i_density,
			mass,
			inertia,
			i_restitution,
			area, i_isStatic,
			0.f,
			i_width,
			i_height,
			vertices,
			EShapeType::Box)
	);

	return body;
}

std::unique_ptr<eae6320::Physics::PhysicsBody2D> eae6320::Physics::CreateBoxBody(float i_width, float i_height, float i_density, bool i_isStatic, float i_restitution)
{
	std::unique_ptr<eae6320::Physics::PhysicsBody2D> body = nullptr;

	float area = i_width * i_height;

	if (area < Physics::cPhysicsWorld::MinBodySize)
	{
		return nullptr;
	}

	if (area > Physics::cPhysicsWorld::MaxBodySize)
	{
		return nullptr;
	}

	if (i_density < Physics::cPhysicsWorld::MinDensity)
	{
		return nullptr;
	}

	if (i_density > Physics::cPhysicsWorld::MaxDensity)
	{
		return nullptr;
	}

	i_restitution = Util::Clamp(i_restitution, 0.f, 1.f);

	float mass = 0.f;
	float inertia = 0.f;

	if (!i_isStatic)
	{
		mass = area * i_density;
		inertia = (1.f / 12.f) * mass * (i_width * i_width + i_height * i_height);
	}

	std::vector<Math::sVector> vertices;
	ConstructBox(i_width, i_height, vertices);

	body = std::unique_ptr<Physics::PhysicsBody2D>(
		new Physics::PhysicsBody2D(
			i_density,
			mass,
			inertia,
			i_restitution,
			area, i_isStatic,
			0.f,
			i_width,
			i_height,
			vertices,
			EShapeType::Box)
	);

	return body;
}
