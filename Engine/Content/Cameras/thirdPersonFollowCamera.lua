--[[
	One Shot's aiming / chase camera

	It follows a point some way down the range from the arrow (OneShotGame/GameplaySettings.h, thirdPersonTargetOffset)
	and sits behind and above that point, always looking at it:
		camera = followed point + shoulderOffset + cameraDistance toward the archer
	With the followed point 14 m down the range, 18 m back puts the camera 4 m behind the arrow,
	1.2 m to the right and 0.9 m above, looking over the archer's shoulder down the range.
	(The last keyframe of the opening shot in GameplaySettings.h is this exact view, so the opening shot ends seamlessly.)
]]

return
{
    camera =
    {
        fieldOfViewRadians = { 0.85, 0.85 },
        -- The near plane is as far away as possible (5 cm) so that the depth buffer has enough precision
        -- to keep the painted lines on the grass from flickering 70 m away
        zNearPlane = 0.05,
        -- The sky dome (280 m) is always centered on the camera, so the far plane must be beyond it
        zFarPlane = 320.0,
    },
    positionStrategy =
    {
        type = "Follow",
		targetUpDirection = { 0.0, 1.0, 0.0 },
		-- How quickly the camera catches up with where it should be on each axis (smaller is quicker)
		damping = { 0.05, 0.05, 0.05 },
		shoulderOffset = { 1.2, 0.9, 0.0 },
		armLength = 0.0,
		cameraDistance = 18.0,
    },
    orientationStrategy =
    {
        type = "HardLookAt",
		cameraUpDirectionOverride = { 0.0, 1.0, 0.0 },
		damping = 0.0,
    },
}
