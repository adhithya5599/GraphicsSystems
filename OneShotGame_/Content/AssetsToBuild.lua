--[[
	This file lists every asset that must be built by the AssetBuildSystem
]]

return
{
	shaders =
	{
		{ path = "Shaders/Vertex/standard.shader", arguments = { "vertex" } },
		{ path = "Shaders/Fragment/standard.shader", arguments = { "fragment" } },

		{ path = "Shaders/Vertex/vertexInputLayout_mesh.shader", arguments = { "vertex" } },
		--{ path = "Shaders/Fragment/myShader.shader", arguments = { "fragment" } },
		--{ path = "Shaders/Fragment/myAnotherShader.shader", arguments = { "fragment" }},
	},

	meshes = 
	{
		{ path = "Meshes/ground.mayamesh"},
		{ path = "Meshes/player.mayamesh" },
		{ path = "Meshes/projectile.mayamesh" },
		{ path = "Meshes/obstacle.mayamesh" },
		{ path = "Meshes/goal.mayamesh" },
		{ path = "Meshes/skybox.mayamesh" },
	},

	textures =
	{
		{ path = "Textures/groundTexture.bmp"},
		{ path = "Textures/playerTexture.bmp" },
		{ path = "Textures/projectileTexture.bmp" },
		{ path = "Textures/goalTexture.bmp" },
		{ path = "Textures/skyboxTexture.bmp" },
		{ path = "textures/obstacleTexture.bmp" },
	},

	cameras = 
	{
		{ path = "Cameras/firstPersonCamera.lua" },
		{ path = "Cameras/thirdPersonFollowCamera.lua" },
	}
}
