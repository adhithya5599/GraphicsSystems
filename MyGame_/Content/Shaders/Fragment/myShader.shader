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

	// Output
	//=======

	// Whatever color value is output from the fragment shader
	// will determine the color of the corresponding pixel on the screen
	out float4 o_color : SV_TARGET

)
#elif defined( EAE6320_PLATFORM_GL )
out vec4 o_color;
void main()
#endif
{
	// Output solid white
	o_color = float4(
		// RGB (color)
		1.0, 1.0, 1.0,
		// Alpha (opacity)
		1.0 );
	o_color.g = 0.0;
	o_color.b = 0.6;
	
	if(sin(g_elapsedSecondCount_simulationTime) < 0)
	{
		o_color.r = 1 - sin(g_elapsedSecondCount_simulationTime);
	}
	else
	{
		o_color.r = sin(g_elapsedSecondCount_simulationTime);
	}
	if(cos(g_elapsedSecondCount_simulationTime) < 0)
	{
		o_color.g = 1 - cos(g_elapsedSecondCount_simulationTime);
	}
	else
	{
		o_color.g = cos(g_elapsedSecondCount_simulationTime);
	}
}
