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
	vec4 height = texture(mTexture4, vertTexcoord.xy);
	float blend = texture(mTexture1, vertTexcoord.zw).r;
	
	vec4 normalspec = texture(mTexture3, vertTexcoord.xy);
	vec3 normal = normalspec.xyz*2.0-1.0;
	mat3 matTangentInv;
	matTangentInv[0] = normalize(vertTangent);
	matTangentInv[1] = normalize(vertBitangent);
	matTangentInv[2] = normalize(vertNormal);
	normal = normalize(matTangentInv*normal);
	normal = mix(normalize(vertNormal), normal, blend);
	
	vec4 color = ((1.0-blend) < height.r*4.0)?color1:color0;
	
	rn_Lighting(color, vec4(0.0), normal, vertPosition);
	
#if defined(RN_FOG)
	float camdist = max(min((length(vertPosition-viewPosition)-fogPlanes.x)*fogPlanes.y, 1.0), 0.0);
	color = mix(color, fogColor, camdist);
#endif
	
	fragColor0 = color;
}
