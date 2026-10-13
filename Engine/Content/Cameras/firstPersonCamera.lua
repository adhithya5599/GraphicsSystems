return
{
    camera =
    {
        fieldOfViewRadians = { 1.0471976, 1.0471976 },
        -- 5 cm keeps the arrow (which starts 55 cm in front of the camera) visible
        -- while giving the depth buffer enough precision for the far end of the range
        zNearPlane = 0.05,
        -- The sky dome (280 m) is always centered on the camera, so the far plane must be beyond it
        zFarPlane = 320.0,
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
