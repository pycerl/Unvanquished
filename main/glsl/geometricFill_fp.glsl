/*
===========================================================================
Copyright (C) 2007-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/* geometricFill_fp.glsl */

#extension GL_ARB_draw_buffers : enable

uniform sampler2D	u_DiffuseMap;
uniform sampler2D	u_NormalMap;
uniform sampler2D	u_SpecularMap;

uniform samplerCube	u_EnvironmentMap0;
uniform samplerCube	u_EnvironmentMap1;
uniform float		u_EnvironmentInterpolation;

uniform float		u_AlphaThreshold;
uniform vec3		u_ViewOrigin;
uniform vec3        u_AmbientColor;
uniform float		u_DepthScale;
uniform mat4		u_ModelMatrix;
uniform vec2		u_SpecularExponent;

varying vec4		var_Position;
varying vec2		var_TexDiffuse;
#if defined(USE_NORMAL_MAPPING)
varying vec2		var_TexNormal;
#if !defined(r_DeferredLighting)
varying vec2		var_TexSpecular;
#endif
varying vec3		var_Tangent;
varying vec3		var_Binormal;
#endif
varying vec3		var_Normal;
varying vec4		var_Color;




void	main()
{
	// compute view direction in world space
	vec3 V = normalize(u_ViewOrigin - var_Position.xyz);

	vec2 texDiffuse = var_TexDiffuse.st;

#if defined(USE_NORMAL_MAPPING)
	// invert tangent space for two sided surfaces
	mat3 tangentToWorldMatrix = mat3(var_Tangent.xyz, var_Binormal.xyz, var_Normal.xyz);

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		tangentToWorldMatrix = -tangentToWorldMatrix;
	}
#endif

	vec2 texNormal = var_TexNormal.st;
#if !defined(r_DeferredLighting)
	vec2 texSpecular = var_TexSpecular.st;
#endif

#if defined(USE_PARALLAX_MAPPING)

	// ray intersect in view direction

	// compute view direction in tangent space
	vec3 Vts = (u_ViewOrigin - var_Position.xyz) * tangentToWorldMatrix;
	Vts = normalize(Vts);

	// size and start position of search in texture space
	vec2 S = Vts.xy * -u_DepthScale / Vts.z;

#if 0
	vec2 texOffset = vec2(0.0);
	for(int i = 0; i < 4; i++) {
		vec4 Normal = texture2D(u_NormalMap, texNormal.st + texOffset);
		float height = Normal.a * 0.2 - 0.0125;
		texOffset += height * Normal.z * S;
	}
#else
	float depth = RayIntersectDisplaceMap(texNormal, S, u_NormalMap);

	// compute texcoords offset
	vec2 texOffset = S * depth;
#endif

	texNormal.st += texOffset;

#if !defined(r_DeferredLighting)
	texDiffuse.st += texOffset;
	texSpecular.st += texOffset;
#endif

#endif // USE_PARALLAX_MAPPING

	// compute normal in world space from normalmap
	vec3 N = normalize(tangentToWorldMatrix * (2.0 * (texture2D(u_NormalMap, texNormal).xyz - 0.5)));

#if !defined(r_DeferredLighting)

	// compute the specular term
#if defined(USE_REFLECTIVE_SPECULAR)

	vec4 specular = texture2D(u_SpecularMap, texSpecular).rgba;

	vec4 envColor0 = textureCube(u_EnvironmentMap0, reflect(-V, N)).rgba;
	vec4 envColor1 = textureCube(u_EnvironmentMap1, reflect(-V, N)).rgba;

	specular *= mix(envColor0, envColor1, u_EnvironmentInterpolation).rgb;

#if 0
	// specular = vec4(u_EnvironmentInterpolation, u_EnvironmentInterpolation, u_EnvironmentInterpolation, 1.0);
	specular.rgb = envColor0;
#endif

#else

	vec4 specular = texture2D(u_SpecularMap, texSpecular).rgba;
#endif // USE_REFLECTIVE_SPECULAR

#endif // #if !defined(r_DeferredLighting)


#else // USE_NORMAL_MAPPING

	vec3 N = normalize(var_Normal);

#if defined(TWOSIDED)
	if(gl_FrontFacing)
	{
		N = -N;
	}
#endif

	vec3 specular = vec3(0.0);

#endif // USE_NORMAL_MAPPING

	// compute the diffuse term
	vec4 diffuse = texture2D(u_DiffuseMap, texDiffuse);

	if( abs(diffuse.a + u_AlphaThreshold) <= 1.0 )
	{
		discard;
		return;
	}

//	vec4 depthColor = diffuse;
//	depthColor.rgb *= u_AmbientColor;

	// convert normal back to [0,1] color space
	N = N * 0.5 + 0.5;

#if defined(r_DeferredLighting)
	gl_FragColor = vec4(N, 0.0);
#else
	gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
	gl_FragData[1] = vec4(diffuse.rgb, var_Color.a);
	gl_FragData[2] = vec4(N, var_Color.a);
	gl_FragData[3] = vec4(specular.rgb, u_SpecularExponent.x * specular.a + u_SpecularExponent.y);

#endif // r_DeferredLighting
}


