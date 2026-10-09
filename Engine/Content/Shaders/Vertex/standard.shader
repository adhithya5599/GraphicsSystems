/*
	This is the standard vertex shader

	A vertex shader is responsible for two things:
		* Telling the GPU where the vertex (one of the three in a triangle) should be drawn on screen in a given window
			* The GPU will use this to decide which fragments (i.e. pixels) need to be shaded for a given triangle
		* Providing any data that a corresponding fragment shader will need
			* This data will be interpolated across the triangle and thus vary for each fragment of a triangle that gets shaded

	For lighting the fragment shader needs each fragment's normal and position in world space
	(world space is where the lights are defined), so both are calculated here and interpolated by the GPU.
*/

#include <Shaders/shaders.inc>

// Constant Buffers
//=================

DeclareConstantBuffer(g_constantBuffer_drawCall, 2)
{
	float4x4 g_transform_localToWorld;
};

#if defined( EAE6320_PLATFORM_D3D )
void main(

	// Input
	//======

	// The "semantics" (the keywords in all caps after the colon) are arbitrary,
	// but must match the C call to CreateInputLayout()

	// These values come from one of the VertexFormats::sVertex_mesh that the vertex buffer was filled with in C code
	in const float3 i_vertexPosition_local : POSITION,
	in const float4 i_vertexColor_local : COLOR,
	in const float2 i_texcoord_local : TEXCOORD,
	in const float3 i_vertexNormal_local : NORMAL,
	// Output
	//=======

	// An SV_POSITION value must always be output from every vertex shader
	// so that the GPU can figure out which fragments need to be shaded
	out float4 o_vertexPosition_projected : SV_POSITION,
	out float4 o_vertexColor_projected : COLOR,
	out float2 o_texcoord_projected : TEXCOORD0,
	// These are interpolated across the triangle for the fragment shader
	// (the semantics just have to match the fragment shader's inputs;
	// TEXCOORD0 is already used by the texture coordinates, so the world position uses TEXCOORD1)
	out float3 o_vertexNormal_world : NORMAL,
	out float3 o_vertexPosition_world : TEXCOORD1

)
#elif defined( EAE6320_PLATFORM_GL )
layout( location = 0 ) in vec3 i_vertexPosition_local;
layout( location = 1 ) in vec4 i_vertexColor_local;
layout( location = 1 ) out vec4 o_vertexColor_projected;
layout( location = 2 ) in vec2 i_texcoord_local;
layout( location = 2 ) out vec2 o_texcoord_projected;
// The locations must match the C calls to glVertexAttribPointer() (inputs)
// and the fragment shader's inputs (outputs)
layout( location = 3 ) in vec3 i_vertexNormal_local;
layout( location = 3 ) out vec3 o_vertexNormal_world;
layout( location = 4 ) out vec3 o_vertexPosition_world;
void main()
#endif
{
	// Transform the local vertex into world space
	float4 vertexPosition_world;
	{
		// This will be done in a future assignment.
		// For now, however, local space is treated as if it is the same as world space.
		float4 vertexPosition_local = float4( i_vertexPosition_local, 1.0 );
		vertexPosition_world = TransfromVector(g_transform_localToWorld, vertexPosition_local);
		o_vertexPosition_world = vertexPosition_world.xyz;
	}
	// Calculate the position of this vertex projected onto the display
	{
		// Transform the vertex from world space into camera space
		float4 vertexPosition_camera = TransfromVector( g_transform_worldToCamera, vertexPosition_world );
		// Project the vertex from camera space into projected space
		o_vertexPosition_projected = TransfromVector( g_transform_cameraToProjected, vertexPosition_camera );
	}
	//Assign input color to the output color
	{
		o_vertexColor_projected = float4(i_vertexColor_local);
	}
	//Assign the texture
	{
		o_texcoord_projected = float2(i_texcoord_local);
	}
	// Transform the normal into world space
	{
		// A w of 0 means "direction": the rotation applies but the translation doesn't.
		// (Strictly, normals should use the inverse transpose of the local-to-world matrix,
		// but game objects in this engine only ever rotate and translate, never scale,
		// and for a rotation the inverse transpose is the matrix itself.)
		float4 vertexNormal_world = TransfromVector( g_transform_localToWorld, float4( i_vertexNormal_local, 0.0 ) );
		o_vertexNormal_world = vertexNormal_world.xyz;
	}
}
