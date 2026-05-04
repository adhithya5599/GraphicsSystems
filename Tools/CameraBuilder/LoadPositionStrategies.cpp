#include "LoadPositionStrategies.h"

namespace eae6320::Assets
{
	template <>
	cResult LoadPositionStrategy<Camera::cPositionComposer>(lua_State& io_luaState, Camera::cPositionComposer& o_positionComposer)
	{
		// top: { type = "Composer", deadZone = { ... }, ... }

		Math::sRectangle deadZone;
		if (LoadRectangle(io_luaState, "deadZone", deadZone))
		{
			o_positionComposer.SetDeadZone(deadZone);
		}

		Math::sRectangle softZone;
		if (LoadRectangle(io_luaState, "softZone", deadZone))
		{
			o_positionComposer.SetSoftZone(deadZone);
		}

		Math::sVector2D targetOffset;
		if (LoadVector2D(io_luaState, "targetOffset", targetOffset))
		{
			o_positionComposer.SetTargetOffset(targetOffset);
		}

		Math::sVector2D damping;
		if (LoadVector2D(io_luaState, "damping", damping))
		{
			o_positionComposer.SetDamping(damping);
		}

		Math::sVector screenPlaneNormalOverride;
		if (LoadVector(io_luaState, "screenPlaneNormalOverride", screenPlaneNormalOverride))
		{
			o_positionComposer.SetScreenPlaneNormalOverride(screenPlaneNormalOverride);
		}

		Math::sVector cameraUpDirectionOverride;
		if (LoadVector(io_luaState, "cameraUpDirectionOverride", cameraUpDirectionOverride))
		{
			o_positionComposer.SetCameraUpDirectionOverride(cameraUpDirectionOverride);
		}

		//

		return Results::Success;
	}

	template <>
	cResult LoadPositionStrategy<Camera::cPositionFollow>(lua_State& io_luaState, Camera::cPositionFollow& o_positionFollow)
	{
		// top: { type = "Follow", targetUpDirection = { ... }, ... }

		Math::sVector targetUpDirection;
		if (LoadVector(io_luaState, "targetUpDirection", targetUpDirection))
		{
			o_positionFollow.SetTargetUpDirection(targetUpDirection);
		}

		Math::sVector damping;
		if (LoadVector(io_luaState, "damping", damping))
		{
			o_positionFollow.SetDamping(damping);
		}

		Math::sVector shoulderOffset;
		if (LoadVector(io_luaState, "shoulderOffset", shoulderOffset))
		{
			o_positionFollow.SetShoulderOffset(shoulderOffset);
		}

		float armLength = 0.f;
		if (LoadFloat(io_luaState, "armLength", armLength))
		{
			o_positionFollow.SetArmLength(armLength);
		}

		float cameraDistance = 0.f;
		if (LoadFloat(io_luaState, "cameraDistance", cameraDistance))
		{
			o_positionFollow.SetCameraDistance(cameraDistance);
		}

		//

		return Results::Success;
	}

	template <>
	cResult LoadPositionStrategy<Camera::cPositionLockToTarget>(lua_State& io_luaState, Camera::cPositionLockToTarget& o_positionLockToTarget)
	{
		// top: { type = "LockToTarget", damping = { ... }, ... }

		Math::sVector damping;
		if (LoadVector(io_luaState, "damping", damping))
		{
			o_positionLockToTarget.SetDamping(damping);
		}

		bool dampingDirectionsRelativeToTarget = false;
		if (LoadBool(io_luaState, "dampingDirectionsRelativeToTarget", dampingDirectionsRelativeToTarget))
		{
			o_positionLockToTarget.SetDampingDirectionsRelativeToTarget(dampingDirectionsRelativeToTarget);
		}

		//

		return Results::Success;
	}

	template <>
	cResult LoadPositionStrategy<Camera::cPositionUnmodified>(lua_State& io_luaState, Camera::cPositionUnmodified& o_positionUnmodified)
	{
		// top: { type = "Unmodified", }

		// cPositionUnmodified has no fields

		return Results::Success;
	}
}