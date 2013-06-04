//
//  rn_Water.fsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;
uniform sampler2D mTexture1;
uniform sampler2D mTexture2;

uniform vec3 viewPosition;
uniform vec2 clipPlanes;

in vec3 vertProjPos;
in vec3 vertPosition;
in vec2 vertTexcoord;

out vec4 fragColor0;

void main()
{
	vec3 normals = normalize(texture(mTexture1, vertTexcoord).xyz*2.0f-1.0f);
	
	vec2 coords = vertProjPos.xy/vertProjPos.z*0.5+0.5;
	vec4 refraction = texture(mTexture2, coords-normals.xy*0.02);
	coords.y = 1.0-coords.y;
	vec4 reflection = texture(mTexture0, coords+normals.xy*0.02);

	vec3 viewdir = normalize(viewPosition-vertPosition);
	float base = 1 - dot(viewdir, vec3(0.0, 1.0, 0.0));
	float exponential = pow(base, 5.0);
	float fresnel = exponential + 0.01 * (1.0 - exponential);
	
	vec2 depth;
	depth.x = refraction.a;
	depth.y = gl_FragCoord.z;
	depth = (clipPlanes.x * clipPlanes.y)/(depth*(clipPlanes.y-clipPlanes.x)-clipPlanes.y);
	float depthdiff = depth.y-depth.x;
	depthdiff *= 0.18;
	
	refraction = mix(refraction, vec4(0.0, 0.0, 0.0, 1.0), min(pow(depthdiff, 1.0/2.2), 1.0));
	vec4 color0 = refraction+reflection*fresnel;
	color0.a = 1.0;
	fragColor0 = refraction;
}
