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
        type = "Unmodified",
    },
    orientationStrategy =
    {
        type = "Composer",
        deadZone = { left = -0.4, right = 0.0, bottom = 0.15, top = 0.35 },
        softZone = { left = -0.8, right = 0.5, bottom = -0.25, top = 0.75 },
        targetOffset = { -0.25, 0.25 },
        damping = 0.5,
        cameraUpDirectionOverride = { 0.0, 1.0, 0.0 },
    },
}
