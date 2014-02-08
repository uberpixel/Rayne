//
//  rn_Water.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;
uniform sampler2D mTexture2;

uniform vec3 viewPosition;
uniform vec2 clipPlanes;
uniform vec4 frameSize;
uniform vec4 cameraAmbient;

in vec3 vertProjPos;
in vec3 vertPosition;
in vec4 vertTexcoord;
in vec4 vertTexcoord2;


#if defined(RN_LIGHTING)
#if defined(RN_DIRECTIONAL_LIGHTS)
	uniform vec3 lightDirectionalDirection[RN_DIRECTIONAL_LIGHTS];
	uniform vec4 lightDirectionalColor[RN_DIRECTIONAL_LIGHTS];
#endif
#endif


out vec4 fragColor0;

void main()
{	
	vec2 coords = vertProjPos.xy/vertProjPos.z*0.5+0.5;

	float depth = texture(mTexture2, coords).a;
	float depth1 = -(2.0f * clipPlanes.y * clipPlanes.x) / (clipPlanes.y - clipPlanes.x)/(depth*2.0-1.0-(clipPlanes.y + clipPlanes.x) / (clipPlanes.y - clipPlanes.x));
	float depth2 = 1.0/gl_FragCoord.w;
	float depthdiff = depth1-depth2;

	vec3 normals = texture(mTexture1, vertTexcoord.xy).xyz*2.0;
	normals += texture(mTexture1, vertTexcoord.zw).xyz*2.0;
	normals += texture(mTexture1, vertTexcoord2.xy).xyz;
	normals += texture(mTexture1, vertTexcoord2.zw).xyz;
	normals -= 3.0;
	normals = normalize(normals);

	vec3 scaledNormals = normals*0.12*min(depthdiff*0.5, 1.0);
	vec4 refraction = texture(mTexture2, coords-scaledNormals.xy);
	coords.y = 1.0-coords.y;
	vec4 reflection = texture(mTexture0, coords+scaledNormals.xy);
	reflection.rgb *= vec3(0.5, 0.4, 0.5);

	vec3 viewdir = normalize(viewPosition-vertPosition);
	float base = 1.0 - dot(viewdir, vec3(0.0, 1.0, 0.0));
	float exponential = pow(base, 5.0);
	float fresnel = exponential + 0.01 * (1.0 - exponential);

	vec3 spec = vec3(0.0);


#if defined(RN_LIGHTING)
#if defined(RN_DIRECTIONAL_LIGHTS)
	for(int i = 0; i < RN_DIRECTIONAL_LIGHTS; i++)
	{
		vec3 halfvec = normalize(lightDirectionalDirection[i]+viewdir-scaledNormals.xzy);
		float specfac = pow(min(max(dot(halfvec, normals.rbg), 0.0), 1.0), 1000.0);
		spec += lightDirectionalColor[i].rgb*specfac;
	}
#endif
#endif


	refraction.rgb *= max(min(exp(-vec3(0.5, 0.5, 0.7)*depthdiff*2.0), 1.0), 0.0);
	vec4 color0 = mix(refraction, vec4(0.4, 0.4, 0.2, 1.0)*cameraAmbient, 0.5)+reflection*fresnel;
	color0.a = 1.0;
	fragColor0 = color0+vec4(spec, 0.0);
}
