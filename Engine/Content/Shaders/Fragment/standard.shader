/*
	This is the standard fragment shader

	A fragment shader is responsible for telling the GPU what color a specific fragment should be
*/

#include <Shaders/shaders.inc>

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
	in const float2 i_fragmentTexture : TEXCOORD,

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
	// Sample the texture
	float4 sampledColor = SamplerTexture(g_colorTexture, i_fragmentTexture);
	o_color = float4(sampledColor * i_fragmentColor);
}
