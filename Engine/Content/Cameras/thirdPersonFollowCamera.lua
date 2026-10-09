return
{
    camera =
    {
        fieldOfViewRadians = { 0.85, 0.85   },
        zNearPlane = 0.01,
        zFarPlane = 300.0,
    },
    positionStrategy =
    {
        type = "Follow",
		targetUpDirection = { 0.0, 1.0, 0.0 },
		--damping = { 0.01, 0.5, 0.1 },
		damping = { 0.05, 0.05, 0.05 },
		--shoulderOffset = { 0.0, 1.5, 0.0 },
		shoulderOffset = { 45.0, 0.0, 0.0 },
		armLength = 0.0,
		--cameraDistance = 4.0,
		cameraDistance = 20.0,
    },
    orientationStrategy =
    {
        type = "HardLookAt",
		cameraUpDirectionOverride = { 0.0, 1.0, 0.0 },
		damping = 0.0,
    },

}
