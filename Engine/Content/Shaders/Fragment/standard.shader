/*
	This is the standard fragment shader

	A fragment shader is responsible for telling the GPU what color a specific fragment should be

	This one is lit: the texture multiplied by the vertex color is the surface's base color ("albedo"),
	and that is lit by the scene's lights (see lighting.inc).
	Lighting is calculated per fragment rather than per vertex
	so that a big, flat triangle (like a floor) can still show a small pool of light from a point light
	and highlights don't depend on how finely a mesh is tessellated.
*/

#include <Shaders/shaders.inc>
#include <Shaders/lighting.inc>

DeclareTexture(g_colorTexture, 0);

#if defined( EAE6320_PLATFORM_D3D )
// Constant Buffers
//=================

// Entry Point
//============
void main(

	// Input
	//======

	in const float4 i_fragmentPosition : SV_POSITION,
	in const float4 i_fragmentColor : COLOR,
	in const float2 i_fragmentTexture : TEXCOORD0,
	in const float3 i_fragmentNormal_world : NORMAL,
	in const float3 i_fragmentPosition_world : TEXCOORD1,

	// Output
	//=======

	// Whatever color value is output from the fragment shader
	// will determine the color of the corresponding pixel on the screen
	out float4 o_color : SV_TARGET

)
#elif defined( EAE6320_PLATFORM_GL )
layout( location = 1 ) in vec4 i_fragmentColor;
layout( location = 2 ) in vec2 i_fragmentTexture;
layout( location = 3 ) in vec3 i_fragmentNormal_world;
layout( location = 4 ) in vec3 i_fragmentPosition_world;
out vec4 o_color;
void main()
#endif
{
	// Sample the texture
	float4 sampledColor = SamplerTexture(g_colorTexture, i_fragmentTexture);
	// The texture tinted by the vertex color is the surface's color under white light.
	// Both are authored in sRGB, but lighting must be calculated in linear space.
	float4 albedo_sRGB = sampledColor * i_fragmentColor;
	float3 albedo_linear = ConvertSrgbToLinear( albedo_sRGB.rgb );
	float3 litColor_linear = CalculateLitColor( albedo_linear, i_fragmentNormal_world, i_fragmentPosition_world );
	// The back buffer isn't an sRGB format, so the conversion back to sRGB must be done here
	// (ConvertLinearToSrgb() clamps anything brighter than 1, which can't be displayed)
	o_color = float4( ConvertLinearToSrgb( litColor_linear ), albedo_sRGB.a );
}
