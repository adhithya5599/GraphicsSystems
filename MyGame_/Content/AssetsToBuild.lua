--[[
	This file lists every asset that must be built by the AssetBuildSystem
]]

return
{
	shaders =
	{
		-- The standard shaders are lit (see Engine/Content/Shaders/lighting.inc)
		{ path = "Shaders/Vertex/standard.shader", arguments = { "vertex" } },
		{ path = "Shaders/Fragment/standard.shader", arguments = { "fragment" } },

		{ path = "Shaders/Vertex/vertexInputLayout_mesh.shader", arguments = { "vertex" } },
		-- These unlit shaders aren't used by any object at the moment
		-- but still compile against the new vertex shader outputs
		{ path = "Shaders/Fragment/myShader.shader", arguments = { "fragment" } },
		{ path = "Shaders/Fragment/myAnotherShader.shader", arguments = { "fragment" }},
	},

	meshes = 
	{
		-- The Maya meshes are no longer drawn, so they aren't built
		-- (uncomment them to use them again; MeshBuilder generates normals for them because .mayamesh files don't have any)
		--{ path = "Meshes/plane.mayamesh" },
		--{ path = "Meshes/cylinder.mayamesh" },
		--{ path = "Meshes/prism.mayamesh" },
		--{ path = "Meshes/testColor.mayamesh" },
		-- Exported from Content/Blender/BlenderScene.blend with Tools/BlenderMeshExporter
		-- (MeshBuilder builds these the same way as the Maya meshes)
		{ path = "Meshes/suzanne.blendermesh" },
		{ path = "Meshes/floor.blendermesh" },
	},

	textures =
	{
		{ path = "Textures/groundTexture.bmp"},
		{ path = "Textures/whiteTexture.bmp" },
		{ path = "Textures/waterTexture.bmp" },
		{ path = "Textures/lensTexture.bmp" },
		{ path = "Textures/grassTexture.bmp" },
	},

	-- The scene's lights.
	-- The game code never loads this: the Graphics system loads "data/Lighting/scene.lighting" itself.
	lighting =
	{
		{ path = "Lighting/scene.lighting" },
	},
}
