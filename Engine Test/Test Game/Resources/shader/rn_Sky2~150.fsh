//
//  rn_Sky.fsh
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#version 150
precision highp float;

uniform sampler2D mTexture0;

uniform vec4 ambient;

#if defined(RN_ATMOSPHERE)
in vec3 vertDirToCam;

//
// Atmospheric scattering vertex shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

#if defined(RN_LIGHTING)
#if defined(RN_DIRECTIONAL_LIGHTS)
uniform int lightDirectionalCount;
uniform vec3 lightDirectionalDirection[RN_DIRECTIONAL_LIGHTS];
uniform vec4 lightDirectionalColor[RN_DIRECTIONAL_LIGHTS];
#define v3LightPos lightDirectionalDirection[0]
#else
vec3 v3LightPos = vec3(0.707, 0.707, 0.0);		// The direction vector to the light source
#endif
#else
vec3 v3LightPos = vec3(0.707, 0.707, 0.0);		// The direction vector to the light source
#endif

vec3 v3CameraPos = vec3(0.0, 10.01, 0.0);		// The camera's current position
vec3 v3InvWavelength = vec3(5.602, 9.473, 19.643);	// 1 / pow(wavelength, 4) for the red, green, and blue channels
float fCameraHeight = 10.000012;	// The camera's current height
float fCameraHeight2 = 100.00025;	// fCameraHeight^2
float fOuterRadius = 10.25;		// The outer (atmosphere) radius
float fOuterRadius2 = 105.0625;	// fOuterRadius^2
float fInnerRadius = 10.0;		// The inner (planetary) radius
float fInnerRadius2 = 100.0;	// fInnerRadius^2
float fKrESun = 0.05;			// Kr * ESun
float fKmESun = 0.02;			// Km * ESun
float fKr4PI = 0.031415;			// Kr * 4 * PI
float fKm4PI = 0.012566;			// Km * 4 * PI
float fScale = 4.0;			// 1 / (fOuterRadius - fInnerRadius)
float fScaleDepth = 0.25;		// The scale depth (i.e. the altitude at which the atmosphere's average density is found)
float fScaleOverScaleDepth = 16.0;	// fScale / fScaleDepth

float g = -0.99;
float g2 = 0.9801;

const int nSamples = 3;
const float fSamples = 3.0;


float scale(float fCos)
{
	float x = 1.0 - fCos;
	return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}
#else
in vec2 vertTexcoord;
#endif

out vec4 fragColor0;

void main()
{
#if defined(RN_ATMOSPHERE)
	vec4 color0 = vec4(1.0, 1.0, 1.0, 1.0);
	
	// Get the ray from the camera to the vertex, and its length (which is the far point of the ray passing through the atmosphere)
	vec3 normVertDirToCam = normalize(vertDirToCam);
	/*if(normVertDirToCam.y < -0.5)
	{
		fragColor0 = color0;
		return;
	}*/
	
	float dirdist = normVertDirToCam.y*fCameraHeight;
	float pointdist = -dirdist+sqrt(dirdist*dirdist-fCameraHeight*fCameraHeight+fOuterRadius2);
	
	vec3 v3Pos = vec3(0.0, fCameraHeight, 0.0)+normVertDirToCam*pointdist;
	vec3 v3Ray = v3Pos - v3CameraPos;
	float fFar = length(v3Ray);
	v3Ray /= fFar;
	
	// Calculate the ray's starting position, then calculate its scattering offset
	vec3 v3Start = v3CameraPos;
	float fHeight = length(v3Start);
	float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight));
	float fStartAngle = dot(v3Ray, v3Start) / fHeight;
	float fStartOffset = fDepth*scale(fStartAngle);
	
	// Initialize the scattering loop variables
	//gl_FrontColor = vec4(0.0, 0.0, 0.0, 0.0);
	float fSampleLength = fFar / fSamples;
	float fScaledLength = fSampleLength * fScale;
	vec3 v3SampleRay = v3Ray * fSampleLength;
	vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;
	
	// Now loop through the sample rays
	vec3 v3FrontColor = vec3(0.0, 0.0, 0.0);
	for(int i=0; i<nSamples; i++)
	{
		float fHeight = length(v3SamplePoint);
		float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
		float fLightAngle = dot(v3LightPos, v3SamplePoint) / fHeight;
		float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
		float fScatter = (fStartOffset + fDepth*(scale(fLightAngle) - scale(fCameraAngle)));
		vec3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
		v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
		v3SamplePoint += v3SampleRay;
	}
	
	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
	vec3 v3Direction = v3CameraPos - v3Pos;
	float fCos = dot(v3LightPos, v3Direction) / length(v3Direction);
	float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);
	color0.xyz = v3FrontColor * (v3InvWavelength * fKrESun) + fMiePhase * v3FrontColor * fKmESun;
	
#else
	vec4 color0 = texture(mTexture0, vertTexcoord)*ambient;
#endif
	fragColor0 = color0;
}

