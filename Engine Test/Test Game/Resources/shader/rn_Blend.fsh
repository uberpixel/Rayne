//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 400

#include <shader/rn_Lighting.fsh>

precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;
uniform sampler2D mTexture2;
uniform sampler2D mTexture3;
uniform sampler2D mTexture4;
uniform sampler2D mTexture5;
uniform sampler2D mTexture6;
uniform sampler2D mTexture7;
uniform sampler2D mTexture8;

#if defined(RN_FOG)
uniform vec2 fogPlanes;
uniform vec4 fogColor;
#endif

in vec4 vertTexcoord;
in vec3 vertPosition;
in vec3 vertNormal;
in vec3 vertBitangent;
in vec3 vertTangent;

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, vertTexcoord.zw*150.0);
	vec4 color1 = texture(mTexture2, vertTexcoord.xy);
	vec4 color2 = texture(mTexture5, vertTexcoord.xy);
	vec4 color3 = texture(mTexture7, vertTexcoord.xy);
	
	vec3 color2norm = texture(mTexture6, vertTexcoord.xy).rgb*2.0-1.0;
	vec3 color3norm = texture(mTexture8, vertTexcoord.xy).rgb*2.0-1.0;
	
	vec4 height = texture(mTexture4, vertTexcoord.xy)*2.0-1.0;
	vec4 blend  = texture(mTexture1, vertTexcoord.zw);
	
	vec4 normalspec = texture(mTexture3, vertTexcoord.xy);
	vec3 normal = normalspec.xyz*2.0-1.0;
	mat3 matTangentInv;
	matTangentInv[0] = normalize(vertTangent);
	matTangentInv[1] = normalize(vertBitangent);
	matTangentInv[2] = normalize(vertNormal);
	
	normal = mix(normal, color2norm, blend.b);
	normal = mix(normal, color3norm, blend.g);
	
	normal = normalize(matTangentInv*normal);
	normal = normalize(mix(normalize(vertNormal), normal, blend.r));
	
	vec4 color = mix(color1, color0, min(max(((1.0-blend.r*2.0)-height.r)*8.0, 0.0), 1.0));
	
	color = mix(color, color2, blend.b);
	color = mix(color, color3, blend.g);
	
	rn_Lighting(color, vec4(0.0), normal, vertPosition);
	
#if defined(RN_FOG)
	float camdist = max(min((length(vertPosition-viewPosition)-fogPlanes.x)*fogPlanes.y, 1.0), 0.0);
	color = mix(color, fogColor, camdist);
#endif
	
	fragColor0 = color;
}
