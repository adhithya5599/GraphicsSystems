--[[
	This file lists every asset that must be built by the AssetBuildSystem
]]

return
{
	shaders =
	{
		-- The standard shaders are lit by the scene's lights (see Engine/Content/Shaders/lighting.inc)
		{ path = "Shaders/Vertex/standard.shader", arguments = { "vertex" } },
		{ path = "Shaders/Fragment/standard.shader", arguments = { "fragment" } },
		-- The sky uses the unlit shader so that it looks the same in every direction
		{ path = "Shaders/Fragment/unlit.shader", arguments = { "fragment" } },

		{ path = "Shaders/Vertex/vertexInputLayout_mesh.shader", arguments = { "vertex" } },
	},

	-- The Maya meshes don't have normals, so MeshBuilder generates them
	meshes =
	{
		{ path = "Meshes/ground.mayamesh" },
		{ path = "Meshes/player.mayamesh" },
		{ path = "Meshes/projectile.mayamesh" },
		{ path = "Meshes/obstacle.mayamesh" },
		{ path = "Meshes/goal.mayamesh" },
		{ path = "Meshes/skybox.mayamesh" },
	},

	textures =
	{
		{ path = "Textures/groundTexture.bmp" },
		{ path = "Textures/playerTexture.bmp" },
		{ path = "Textures/projectileTexture.bmp" },
		{ path = "Textures/goalTexture.bmp" },
		{ path = "Textures/skyboxTexture.bmp" },
		{ path = "Textures/obstacleTexture.bmp" },
	},

	-- The camera files are shared with other games, so they are in Engine/Content/Cameras
	cameras =
	{
		{ path = "Cameras/firstPersonCamera.lua" },
		{ path = "Cameras/thirdPersonFollowCamera.lua" },
	},

	-- The scene's lights.
	-- The game code never loads this: the Graphics system loads "data/Lighting/scene.lighting" itself.
	lighting =
	{
		{ path = "Lighting/scene.lighting" },
	},
}
