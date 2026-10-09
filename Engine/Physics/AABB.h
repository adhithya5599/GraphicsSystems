#pragma once

#include <Engine/Math/sVector2.h>
namespace eae6320
{
	namespace Physics
	{
		struct AABB
		{
            Math::sVector2 Min; 
            Math::sVector2 Max; 

            AABB(const Math::sVector2& i_min, const Math::sVector2& i_max)
                : Min(i_min), Max(i_max) {}

            AABB(float i_minX, float i_minY, float i_maxX, float i_maxY)
                : Min(i_minX, i_minY), Max(i_maxX, i_maxY) {}

            AABB(const AABB& i_other)
                : Min(i_other.Min), Max(i_other.Max) {}

            AABB(AABB&& i_other) noexcept
                : Min(i_other.Min), Max(i_other.Max) {}

            AABB& operator=(const AABB& i_other)
            {
                if (this != &i_other)
                {
                    Min = i_other.Min;
                    Max = i_other.Max;
                }
                return *this;
            }

            AABB& operator=(AABB&& i_other) noexcept
            {
                if (this != &i_other)
                {
                    Min = i_other.Min;
                    Max = i_other.Max;
                }
                return *this;
            }
		};
	}
}