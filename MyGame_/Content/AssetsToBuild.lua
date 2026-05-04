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
		{ path = "Shaders/Fragment/myShader.shader", arguments = { "fragment" } },
		{ path = "Shaders/Fragment/myAnotherShader.shader", arguments = { "fragment" }},
	},

	meshes = 
	{
		{ path = "Meshes/plane.mayamesh" },
		{ path = "Meshes/cylinder.mayamesh" },
		{ path = "Meshes/prism.mayamesh" },
		{ path = "Meshes/testColor.mayamesh" },
	},

	textures =
	{
		{ path = "Textures/groundTexture.bmp"},
		{ path = "Textures/whiteTexture.bmp" },
		{ path = "Textures/waterTexture.bmp" },
		{ path = "Textures/lensTexture.bmp" },
		{ path = "Textures/grassTexture.bmp" },
	}
}
