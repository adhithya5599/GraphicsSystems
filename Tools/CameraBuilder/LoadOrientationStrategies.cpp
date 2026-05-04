#include "LoadOrientationStragies.h"

namespace eae6320::Assets
{
	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationComposer>(lua_State& io_luaState, Camera::cOrientationComposer& o_orientationComposer)
	{
		// top: { type = "Composer", deadZone = { ... }, ... }

		Math::sRectangle deadZone;
		if (LoadRectangle(io_luaState, "deadZone", deadZone))
		{
			o_orientationComposer.SetDeadZone(deadZone);
		}

		Math::sRectangle softZone;
		if (LoadRectangle(io_luaState, "softZone", deadZone))
		{
			o_orientationComposer.SetSoftZone(deadZone);
		}

		Math::sVector2D targetOffset;
		if (LoadVector2D(io_luaState, "targetOffset", targetOffset))
		{
			o_orientationComposer.SetTargetOffset(targetOffset);
		}

		Math::sVector cameraUpDirectionOverride;
		if (LoadVector(io_luaState, "cameraUpDirectionOverride", cameraUpDirectionOverride))
		{
			o_orientationComposer.SetCameraUpDirectionOverride(cameraUpDirectionOverride);
		}

		float damping;
		if (LoadFloat(io_luaState, "damping", damping))
		{
			o_orientationComposer.SetDamping(damping);
		}

		//

		return Results::Success;
	}

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationHardLookAt>(lua_State& io_luaState, Camera::cOrientationHardLookAt& o_orientationHardLookAt)
	{
		// top: { type = "HardLookAt", damping = { ... }, ... }

		Math::sVector cameraUpDirectionOverride;
		if (LoadVector(io_luaState, "cameraUpDirectionOverride", cameraUpDirectionOverride))
		{
			o_orientationHardLookAt.SetCameraUpDirectionOverride(cameraUpDirectionOverride);
		}

		float damping;
		if (LoadFloat(io_luaState, "damping", damping))
		{
			o_orientationHardLookAt.SetDamping(damping);
		}

		//

		return Results::Success;
	}

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationLockToTarget>(lua_State& io_luaState, Camera::cOrientationLockToTarget& o_orientationLockToTarget)
	{
		// top: { type = "LockToTarget", damping = { ... }, ... }

		Math::sVector cameraUpDirectionOverride;
		if (LoadVector(io_luaState, "cameraUpDirectionOverride", cameraUpDirectionOverride))
		{
			o_orientationLockToTarget.SetCameraUpDirectionOverride(cameraUpDirectionOverride);
		}

		float damping;
		if (LoadFloat(io_luaState, "damping", damping))
		{
			o_orientationLockToTarget.SetDamping(damping);
		}

		//

		return Results::Success;
	}

	template <>
	cResult LoadOrientationStrategy<Camera::cOrientationUnmodified>(lua_State& io_luaState, Camera::cOrientationUnmodified& o_orientationUnmodified)
	{
		// top: { type = "Unmodified", }

		// cOrientationUnmodified has no fields

		return Results::Success;
	}
}