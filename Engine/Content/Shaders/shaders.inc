/*
	This file should be #included by all shaders
*/

// Version Information
//====================

#if defined( EAE6320_PLATFORM_GL )

	// GLSL shaders require the version to be #defined before anything else in the shader
	#version 420

#endif

// Constant Buffers
//=================

#if defined( EAE6320_PLATFORM_D3D )

	#define DeclareConstantBuffer( i_name, i_id ) cbuffer i_name : register( b##i_id )
	#define mat4 float4x4
	#define vec2 float2
	#define vec3 float3
	#define vec4 float4
	#define TransfromVector(i_vector1, i_vector2) mul(i_vector1, i_vector2)
	// (GLSL is missing some HLSL functions; the GL versions are defined below)
	#define gl_Position o_vertexPosition_projected
	#define DeclareTexture( i_name, i_id ) Texture2D i_name : register( t##i_id );SamplerState i_name##_Sampler : register( s##i_id)
	#define SamplerTexture( i_name, i_uv ) i_name.Sample( i_name##_Sampler, i_uv )

#elif defined( EAE6320_PLATFORM_GL )

	#define DeclareConstantBuffer( i_name, i_id ) layout( std140, binding = i_id ) uniform i_name
	#define float4x4 mat4
	#define float2 vec2
	#define float3 vec3
	#define float4 vec4
	// The parentheses matter: without them TransfromVector( m, v ).xyz would expand to m * v.xyz
	#define TransfromVector(i_vector1, i_vector2) ( ( i_vector1 ) * ( i_vector2 ) )
	// HLSL functions that GLSL calls by different names
	#define saturate( i_value ) clamp( i_value, 0.0, 1.0 )
	#define lerp mix
	#define o_vertexPosition_projected gl_Position
	#define DeclareTexture( i_name, i_id ) layout( binding = i_id ) uniform sampler2D i_name
	#define SamplerTexture( i_name, i_uv ) texture( i_name, i_uv )

#endif

DeclareConstantBuffer(g_constantBuffer_frame, 0)
{
	float4x4 g_transform_worldToCamera;
	float4x4 g_transform_cameraToProjected;

	float g_elapsedSecondCount_systemTime;
	float g_elapsedSecondCount_simulationTime;
	// For float4 alignment
	float2 g_padding;

	// The camera's position in world space (w is unused)
	float4 g_cameraPosition_world;
};