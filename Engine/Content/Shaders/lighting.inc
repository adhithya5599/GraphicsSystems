/*
	This file contains everything a shader needs in order to be lit:
	the lighting constant buffer and the functions that calculate lighting

	#include it after shaders.inc in any fragment shader that should be lit.
	The data comes from the game's lighting file (e.g. MyGame_/Content/Lighting/scene.lighting),
	which the Graphics system loads and copies into this constant buffer.

	The lighting model:
		* Hemisphere ambient: a blend between a sky color and a ground color based on which way a surface faces
		* Lambert diffuse: a surface is lit in proportion to the cosine of the angle between its normal and the light
		* Blinn-Phong specular: a highlight where the "half vector" between the light and the camera lines up with the normal
	All of the math happens in linear color space (see ConvertSrgbToLinear()).
*/

#ifndef EAE6320_LIGHTING_INC
#define EAE6320_LIGHTING_INC

// This must match Lighting::MaxPointLightCount in Engine/Lighting/sLightingData.h
#define EAE6320_LIGHTING_MAXPOINTLIGHTCOUNT 4

// Constant Buffers
//=================

// This must match ConstantBufferFormats::sLighting in Engine/Graphics/ConstantBufferFormats.h
// (every member is a float4 or one of a group of four scalars
// so that HLSL and GLSL lay it out identically)
DeclareConstantBuffer( g_constantBuffer_lighting, 3 )
{
	float4 g_ambient_skyColor;	// rgb = color * intensity
	float4 g_ambient_groundColor;	// rgb = color * intensity
	float4 g_directional_directionToLight;	// xyz = unit vector pointing toward the light
	float4 g_directional_color;	// rgb = color * intensity

	float g_specular_intensity;
	float g_specular_shininess;
	int g_pointLightCount;
	float g_padding_lighting;

	float4 g_pointLight_positionAndRange[EAE6320_LIGHTING_MAXPOINTLIGHTCOUNT];	// xyz = position, w = range
	float4 g_pointLight_color[EAE6320_LIGHTING_MAXPOINTLIGHTCOUNT];	// rgb = color * intensity
};

// Color Space
//============

// Colors are authored and displayed in "sRGB", which spends more of its values on dark shades
// because that is where human eyes are most sensitive.
// Light, however, adds up linearly (two lights are twice as bright as one),
// so colors are converted to linear before lighting and back to sRGB afterwards.
// (Skipping this makes lit objects look too dark with harsh, quickly-falling-off shading.
// Raising to the power 2.2 is a close and cheap approximation of the exact sRGB curve.)
// pow() is undefined for a negative base, and the Direct3D shader compiler reports an error (warning X3571 treated as an error)
// unless it can prove that the base isn't negative, so the colors are clamped with saturate() first.
// That doesn't change the result: colors are already in [0,1] going in,
// and anything above 1 coming out can't be displayed anyway.
float3 ConvertSrgbToLinear( float3 i_color_sRGB )
{
	return pow( saturate( i_color_sRGB ), float3( 2.2, 2.2, 2.2 ) );
}

float3 ConvertLinearToSrgb( float3 i_color_linear )
{
	return pow( saturate( i_color_linear ), float3( 1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2 ) );
}

// Lighting
//=========

// This adds the diffuse and specular contribution of a single light
void AccumulateLight( float3 i_normal, float3 i_directionToCamera, float3 i_directionToLight, float3 i_lightColor,
	inout float3 io_diffuse, inout float3 io_specular )
{
	// Lambert diffuse:
	// a surface facing the light gets all of it, a surface at 60 degrees gets half, and a surface facing away gets none
	float normalDotLight = saturate( dot( i_normal, i_directionToLight ) );
	io_diffuse += i_lightColor * normalDotLight;

	// Blinn-Phong specular:
	// the "half vector" is halfway between the direction to the light and the direction to the camera.
	// When it lines up with the normal the surface reflects the light straight at the camera.
	// Raising the cosine to the shininess power makes the highlight tighter as shininess increases.
	float3 halfVector = normalize( i_directionToLight + i_directionToCamera );
	float normalDotHalf = saturate( dot( i_normal, halfVector ) );
	// A surface that faces away from the light can't reflect it
	// (normalDotHalf came from saturate(), so the compiler knows this pow() base isn't negative)
	float specularAmount = ( normalDotLight > 0.0 ) ? pow( normalDotHalf, g_specular_shininess ) : 0.0;
	io_specular += i_lightColor * ( specularAmount * g_specular_intensity );
}

// This calculates the final lit color of a surface (in linear color space)
//	* i_albedo_linear is the surface's color under white light
//	* i_normal_world and i_position_world are interpolated from the vertex shader
float3 CalculateLitColor( float3 i_albedo_linear, float3 i_normal_world, float3 i_position_world )
{
	// Interpolating between unit-length vertex normals produces shorter normals in the middle of a triangle,
	// so the normal must be re-normalized per fragment
	float3 normal = normalize( i_normal_world );
	float3 directionToCamera = normalize( g_cameraPosition_world.xyz - i_position_world );

	// Hemisphere ambient:
	// a normal pointing straight up (y = 1) gets the sky color, straight down (y = -1) the ground color
	float skyWeight = 0.5 + ( 0.5 * normal.y );
	float3 ambient = lerp( g_ambient_groundColor.rgb, g_ambient_skyColor.rgb, skyWeight );

	float3 diffuse = float3( 0.0, 0.0, 0.0 );
	float3 specular = float3( 0.0, 0.0, 0.0 );

	// The directional light
	AccumulateLight( normal, directionToCamera, g_directional_directionToLight.xyz, g_directional_color.rgb, diffuse, specular );

	// The point lights
	for ( int i = 0; i < g_pointLightCount; ++i )
	{
		float3 toLight = g_pointLight_positionAndRange[i].xyz - i_position_world;
		// (This isn't called "distance" because that is the name of a built-in function in both HLSL and GLSL)
		float distanceToLight = length( toLight );
		float range = g_pointLight_positionAndRange[i].w;
		// The falloff is inverse-square (physically correct: light spreads over the surface of a growing sphere)
		// with two adjustments:
		//	* The +1 keeps the light from becoming infinitely bright at a distance of zero
		//	* The "window" smoothly forces the light to exactly zero at its range,
		//		so a light has no effect at all beyond its range
		//		(which would let a renderer skip it entirely for objects that are far enough away)
		float distanceRatio = distanceToLight / range;
		float distanceRatio_squared = distanceRatio * distanceRatio;
		float window = saturate( 1.0 - ( distanceRatio_squared * distanceRatio_squared ) );
		float attenuation = ( window * window ) / ( ( distanceToLight * distanceToLight ) + 1.0 );
		float3 directionToLight = toLight / max( distanceToLight, 0.0001 );
		AccumulateLight( normal, directionToCamera, directionToLight, g_pointLight_color[i].rgb * attenuation, diffuse, specular );
	}

	// Diffuse light (ambient included) is tinted by the surface's color,
	// but specular highlights are a reflection of the light itself and so aren't
	return ( i_albedo_linear * ( ambient + diffuse ) ) + specular;
}

#endif	// EAE6320_LIGHTING_INC
