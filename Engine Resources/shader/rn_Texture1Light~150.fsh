//
//  rn_Texture1.fsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;

uniform samplerBuffer lightListColor;
uniform samplerBuffer lightListPosition;
uniform isamplerBuffer lightList;
uniform isamplerBuffer lightListOffset;
uniform vec4 lightTileSize;
uniform vec4 frameSize;

in vec2 outTexcoord;
in vec3 outNormal;
in vec3 outPosition;

out vec4 fragColor0;

void main()
{
	vec4 color0 = texture(mTexture0, outTexcoord);
	vec3 normal = normalize(outNormal);
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0;
	vec3 light = vec3(0.1);
	vec4 lightpos;
	vec3 lightcolor;
	int lightindex = 0;
	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	ivec2 listoffset = texelFetch(lightListOffset, tileindex).xy;
	for(int i = 0; i < listoffset.y; i++)
	{
		lightindex = texelFetch(lightList, listoffset.x+i).r;
		lightpos = texelFetch(lightListPosition, lightindex);
		lightcolor = texelFetch(lightListColor, lightindex).xyz;
		posdiff = lightpos.xyz-outPosition;
		attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);
		light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
	}
	
	color0.rgb *= light;
	if(listoffset.y > 20)
		color0.rgb = vec3(color0.r, 0.0, 0.0);
	else if(listoffset.y > 15)
		color0.rgb = vec3(0.0, color0.g, 0.0);
	else if(listoffset.y > 10)
		color0.rgb = vec3(0.0, 0.0, color0.b);
	else if(listoffset.y > 5)
		color0.rgb = vec3(0.0, color0.g, color0.b);
	fragColor0 = color0;
}
