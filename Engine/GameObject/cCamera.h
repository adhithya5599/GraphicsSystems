#pragma once

/*
	This class is for a Camera
*/

#ifndef EAE6320_GAMEOBJECT_CCAMERA_H
#define EAE6320_GAMEOBJECT_CCAMERA_H

#include <Engine/Results/Results.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector.h>

namespace eae6320
{
	namespace Physics
	{
		struct sRigidBodyState;
	}

	namespace Math
	{
		class cMatrix_transformation;
	}
}

namespace eae6320
{
	namespace GameObject
	{
		class cCamera
		{
			public:
				cCamera();
				~cCamera();

				const Math::cMatrix_transformation GetCameraTransform();
				//const Math::sVector GetCameraPosition() const;
				//const Math::cQuaternion GetCameraRotation() const;
				const Math::cMatrix_transformation GetCameraConfigurations();

				Physics::sRigidBodyState*& GetRigidBodyState();

			private:
				Physics::sRigidBodyState* m_rigidBodyState;
				Math::cMatrix_transformation* m_transform;
				Math::cMatrix_transformation* m_configurations;
		};
	}
}

#endif