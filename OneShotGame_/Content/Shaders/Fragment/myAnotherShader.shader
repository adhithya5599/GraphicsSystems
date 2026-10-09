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
	// Output solid white
	//o_color = float4(
		// RGB (color)
		//1.0, 1.0, 1.0,
		// Alpha (opacity)
		//1.0 );
	float4 calculatedColor = float4(1.0, 1.0, 1.0, 1.0);
	calculatedColor.g = 0.0;
	calculatedColor.b = 1.0;
	calculatedColor.r = 0.0;
	//Vector multiplication for getting combined color
	float r = TransfromVector(calculatedColor.r, i_fragmentColor.r);
	float g = TransfromVector(calculatedColor.g, i_fragmentColor.g);
	float b = TransfromVector(calculatedColor.b, i_fragmentColor.b);
	float a = TransfromVector(calculatedColor.a, i_fragmentColor.a);
	o_color = float4(r, g, b, a);
}
