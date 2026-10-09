return
{
    camera =
    {
        fieldOfViewRadians = { 1.0471976, 1.0471976 },
        zNearPlane = 0.01,
        -- The sky box is a cube 150 units from the camera to each side,
        -- so its corners are about 260 units away (150 * the square root of 3);
        -- a far plane closer than that cuts the corners off the sky
        zFarPlane = 300.0,
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
