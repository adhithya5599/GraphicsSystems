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
		-- The sky uses the unlit shader (the sky is the light source, not something being lit)
		{ path = "Shaders/Fragment/unlit.shader", arguments = { "fragment" } },

		{ path = "Shaders/Vertex/vertexInputLayout_mesh.shader", arguments = { "vertex" } },
	},

	-- Every mesh is made by Content/Blender/build_archery_range.py (which builds Content/Blender/ArcheryRange.blend)
	-- and exported with Tools/BlenderMeshExporter
	meshes =
	{
		{ path = "Meshes/arrow.blendermesh" },
		{ path = "Meshes/bunker.blendermesh" },
		{ path = "Meshes/crowd_0.blendermesh" },
		{ path = "Meshes/crowd_1.blendermesh" },
		{ path = "Meshes/crowd_2.blendermesh" },
		{ path = "Meshes/flag.blendermesh" },
		{ path = "Meshes/flagPole.blendermesh" },
		{ path = "Meshes/popupBoard.blendermesh" },
		{ path = "Meshes/pulley_board.blendermesh" },
		{ path = "Meshes/pulley_frame.blendermesh" },
		{ path = "Meshes/quiver.blendermesh" },
		{ path = "Meshes/railSlider_frame.blendermesh" },
		{ path = "Meshes/railSlider_plank.blendermesh" },
		{ path = "Meshes/range_ground_center.blendermesh" },
		{ path = "Meshes/range_ground_left.blendermesh" },
		{ path = "Meshes/range_ground_outer.blendermesh" },
		{ path = "Meshes/range_ground_right.blendermesh" },
		{ path = "Meshes/range_lines.blendermesh" },
		{ path = "Meshes/range_stands.blendermesh" },
		{ path = "Meshes/range_surroundings.blendermesh" },
		{ path = "Meshes/rope.blendermesh" },
		{ path = "Meshes/scoreboard_display.blendermesh" },
		{ path = "Meshes/scoreboard_frame.blendermesh" },
		{ path = "Meshes/scoreGlyph_0.blendermesh" },
		{ path = "Meshes/scoreGlyph_1.blendermesh" },
		{ path = "Meshes/scoreGlyph_2.blendermesh" },
		{ path = "Meshes/scoreGlyph_3.blendermesh" },
		{ path = "Meshes/scoreGlyph_4.blendermesh" },
		{ path = "Meshes/scoreGlyph_5.blendermesh" },
		{ path = "Meshes/scoreGlyph_6.blendermesh" },
		{ path = "Meshes/scoreGlyph_7.blendermesh" },
		{ path = "Meshes/scoreGlyph_8.blendermesh" },
		{ path = "Meshes/scoreGlyph_9.blendermesh" },
		{ path = "Meshes/scoreGlyph_dash.blendermesh" },
		{ path = "Meshes/scoreGlyph_M.blendermesh" },
		{ path = "Meshes/shooter.blendermesh" },
		{ path = "Meshes/skyDome.blendermesh" },
		{ path = "Meshes/sled_plank.blendermesh" },
		{ path = "Meshes/sled_track.blendermesh" },
		{ path = "Meshes/target.blendermesh" },
		{ path = "Meshes/timingLight_frame.blendermesh" },
		{ path = "Meshes/timingLight_green.blendermesh" },
		{ path = "Meshes/timingLight_label.blendermesh" },
		{ path = "Meshes/timingLight_red.blendermesh" },
	},

	-- Every texture is made by Content/Blender/make_textures.py
	textures =
	{
		{ path = "Textures/grass.bmp" },
		{ path = "Textures/scoreboard.bmp" },
		{ path = "Textures/target.bmp" },
		{ path = "Textures/white.bmp" },
		{ path = "Textures/wood.bmp" },
	},

	-- Every sound is made by Content/Sounds/make_sounds.py
	-- (SoundBuilder turns each .wav into a .sound file, so the game loads e.g. "data/Sounds/bowRelease.sound")
	sounds =
	{
		{ path = "Sounds/bowRelease.wav" },
		{ path = "Sounds/bullseye.wav" },
		{ path = "Sounds/crowdAmbience.wav" },
		{ path = "Sounds/crowdCheer.wav" },
		{ path = "Sounds/crowdGroan.wav" },
		{ path = "Sounds/introFanfare.wav" },
		{ path = "Sounds/obstacleHit.wav" },
		{ path = "Sounds/roundComplete.wav" },
		{ path = "Sounds/targetHit.wav" },
		{ path = "Sounds/whistle.wav" },
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
