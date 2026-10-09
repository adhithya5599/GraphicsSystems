#pragma once

//#include <Jolt/Jolt.h>
//#include <Jolt/Math/Vec3.h>
//#include <Jolt/Math/Quat.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/sVector2.h>
#include <assert.h>

//inline JPH::Vec3 ToJolt(const eae6320::Math::sVector& i_Vector)
//{
//    return JPH::Vec3(i_Vector.x, i_Vector.y, -i_Vector.z);
//}
//
//inline eae6320::Math::sVector ToEngine(const JPH::Vec3& i_Vector)
//{
//    return eae6320::Math::sVector(i_Vector.GetX(), i_Vector.GetY(), -i_Vector.GetZ()); 
//}
//
//inline JPH::Quat ToJolt(const eae6320::Math::cQuaternion& i_Quat)
//{
//    return JPH::Quat(
//        i_Quat.GetX(),
//        i_Quat.GetY(),
//        -i_Quat.GetZ(), 
//        -i_Quat.GetW()
//    );
//}
//
//inline eae6320::Math::cQuaternion ToEngine(const JPH::Quat& i_Quat)
//{
//    return eae6320::Math::cQuaternion(
//        -i_Quat.GetW(),  
//        i_Quat.GetX(),
//        i_Quat.GetY(),
//        -i_Quat.GetZ() 
//    );
//}


// Math
// ===============================
namespace Util
{
    using namespace eae6320::Math;

    inline eae6320::Math::sVector ToVec3(const sVector2& i_vec2)
    {
        return sVector(i_vec2.x, i_vec2.y, 0.f);
    }

    inline eae6320::Math::sVector2 ToVec2(const sVector& i_vec3)
    {
        return sVector2(i_vec3.x, i_vec3.y);
    }

    inline int Clamp(int value, int min, int max)
    {
        if (min == max)
        {
            return min;
        }

        assert(min <= max);

        if (value < min)
        {
            return min;
        }
        if (value > max)
        {
            return max;
        }
        return value;
    }

    inline float Clamp(float value, float min, float max)
    {
        if (min == max)
        {
            return min;
        }

        assert(min <= max);

        if (value < min)
        {
            return min;
        }
        if (value > max)
        {
            return max;
        }
        return value;
    }



    inline void Transform2D(sVector2 i_position, float i_rotation, sVector2& o_vector)
    {
        float cosA = std::cos(i_rotation);
        float sinA = std::sin(i_rotation);

        float rotatedX = o_vector.x * cosA - o_vector.y * sinA;
        float rotatedY = o_vector.x * sinA + o_vector.y * cosA;

        o_vector.x = rotatedX + i_position.x;
        o_vector.y = rotatedY + i_position.y;
    }

	inline sVector ConvertPosition2DTo3D(
		const sVector2& i_position2D,
		float i_zDepth = 0.0f)
	{
		return sVector(i_position2D.x, i_position2D.y, i_zDepth);
	}

	inline cQuaternion ConvertRotation2DTo3D(float i_rotation2D)
	{
		const float halfAngle = i_rotation2D * 0.5f;
		const float sinHalf = std::sin(halfAngle);
		const float cosHalf = std::cos(halfAngle);

		return cQuaternion(cosHalf, 0.0f, 0.0f, sinHalf);
	}


	inline cMatrix_transformation CreateTransform2DTo3D(
		const sVector2& i_position2D,
		float i_rotation2D,
		float i_zDepth = 0.0f)
	{
		cQuaternion rotation = ConvertRotation2DTo3D(i_rotation2D);
		sVector position = ConvertPosition2DTo3D(i_position2D, i_zDepth);

		return cMatrix_transformation(rotation, position);
	}


	inline cMatrix_transformation CreateInterpolatedTransform2DTo3D(
		const sVector2& i_position2D,
		const sVector2& i_velocity2D,
		float i_rotation2D,
		float i_rotationalVelocity,
		float i_interpolationTime,
		float i_zDepth = 0.0f)
	{
		// Interpolate position
	    sVector2 interpolatedPos = i_position2D + i_velocity2D * i_interpolationTime;

		// Interpolate rotation
		float interpolatedRot = i_rotation2D + i_rotationalVelocity * i_interpolationTime;

		return CreateTransform2DTo3D(interpolatedPos, interpolatedRot, i_zDepth);
	}

	// ============================================
	// 3D to 2D Extraction (if needed)
	// ============================================

	/**
	 * Extract 2D position from 3D vector (drops Z component)
	 * @param i_position3D - 3D position
	 * @return 2D position (X and Y only)
	 */
	inline sVector2 Extract2DPosition(const sVector& i_position3D)
	{
		return sVector2(i_position3D.x, i_position3D.y);
	}

	/**
	 * Extract 2D rotation from 3D quaternion
	 * Assumes rotation is around Z-axis
	 * @param i_rotation3D - 3D quaternion
	 * @return 2D rotation angle in radians
	 */
	inline float Extract2DRotation(const cQuaternion& i_rotation3D)
	{

		return 2.0f * std::atan2(i_rotation3D.GetZ(), i_rotation3D.GetW());
	}
}