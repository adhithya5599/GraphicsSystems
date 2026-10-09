return
{
    camera =
    {
        fieldOfViewRadians = { 1.0471976, 1.0471976 },
        zNearPlane = 0.01,
        zFarPlane = 200.0,
    },
    positionStrategy =
    {
        type = "LockToTarget",
        damping = { 0.0, 0.0, 0.0 },
        dampingDirectionsRelativeToTarget = true,
    },
    orientationStrategy =
    {
        type = "Unmodified",
		--cameraUpDirectionOverride = { 0.0, 0.0, 1.0 },
		cameraUpDirectionOverride = { 0.0, 1.0, 0.0 },
		damping = 0.0,
    },
}
