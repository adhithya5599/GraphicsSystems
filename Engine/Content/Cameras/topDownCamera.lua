return
{
    camera =
    {
        fieldOfViewRadians = { 0.0, 0.0 },
        zNearPlane = 0.1,
        zFarPlane = 100.0,
    },
    positionStrategy =
    {
        type = "Composer",
        deadZone = { left = -0.25, right = 0.25, bottom = -0.25, top = 0.25 },
        softZone = { left = -0.75, right = 0.75, bottom = -0.75, top = 0.75 },
        targetOffset = { 0.5, 0.5 },
        damping = { 0.5, 0.5 },
        screenPlaneNormalOverride = { 0.0, 1.0, 0.0 },
        cameraUpDirectionOverride = { 0.0, 0.0, 0.0 },
    },
    orientationStrategy =
    {
        type = "Unmodified",
    },
}
