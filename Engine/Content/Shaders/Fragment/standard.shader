/*
	This is the standard fragment shader

	A fragment shader is responsible for telling the GPU what color a specific fragment should be
*/

#include <Shaders/shaders.inc>

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

	// Output
	//=======

	// Whatever color value is output from the fragment shader
	// will determine the color of the corresponding pixel on the screen
	out float4 o_color : SV_TARGET

)
#elif defined( EAE6320_PLATFORM_GL )
layout( location = 1 ) in vec4 i_fragmentColor;
out vec4 o_color;
void main()
#endif
{
	o_color = float4(i_fragmentColor);

	// Output solid white
	//o_color = float4(
			// RGB (color)
			//1.0, 1.0, 1.0,
			// Alpha (opacity)
			//1.0
		//);
}
