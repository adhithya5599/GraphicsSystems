/*
	This is an unlit fragment shader

	The color is just the texture multiplied by the vertex color, without any lighting.
	This is for things that shouldn't react to the scene's lights,
	like a sky: it is supposed to be the light source (or infinitely far away),
	so shading it like a nearby object would make the inside of the sky box look like a dim, shaded room.

	It uses the same vertex shader (Vertex/standard.shader) as the lit shader.
*/

#include <Shaders/shaders.inc>

DeclareTexture(g_colorTexture, 0);

#if defined( EAE6320_PLATFORM_D3D )

// Entry Point
//============
void main(

	// Input
	//======

	in const float4 i_fragmentPosition : SV_POSITION,
	in const float4 i_fragmentColor : COLOR,
	in const float2 i_fragmentTexture : TEXCOORD0,

	// Output
	//=======

	// Whatever color value is output from the fragment shader
	// will determine the color of the corresponding pixel on the screen
	out float4 o_color : SV_TARGET

)
#elif defined( EAE6320_PLATFORM_GL )
layout( location = 1 ) in vec4 i_fragmentColor;
layout( location = 2 ) in vec2 i_fragmentTexture;
out vec4 o_color;
void main()
#endif
{
	// The texture and vertex color are both authored in sRGB and the back buffer expects sRGB,
	// so (unlike the lit shader) no conversion to linear space and back is needed when nothing is being lit
	o_color = SamplerTexture(g_colorTexture, i_fragmentTexture) * i_fragmentColor;
}
