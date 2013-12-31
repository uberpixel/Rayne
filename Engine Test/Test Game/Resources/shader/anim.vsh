//
//	Shader.vsh
//	iSDGE
//
//	Created by Nils Daumann on 16.04.10.
//	Copyright (c) 2010 Nils Daumann

//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:

//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.

//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#version 150
precision highp float;
in vec3 vertPosition;
in vec2 vertTexcoord0;
in vec4 vertBoneWeights;
in vec4 vertBoneIndices;

uniform mat4 matProjViewModel;
uniform float Time;
uniform mat4 matBones[60];

out vec2 texcoord;

void main()
{
	texcoord = vertTexcoord0;
	
	vec4 pos1 = matBones[int(vertBoneIndices.x)]*vec4(vertPosition, 1.0);
	vec4 pos2 = matBones[int(vertBoneIndices.y)]*vec4(vertPosition, 1.0);
	vec4 pos3 = matBones[int(vertBoneIndices.z)]*vec4(vertPosition, 1.0);
	vec4 pos4 = matBones[int(vertBoneIndices.w)]*vec4(vertPosition, 1.0);
	vec4 pos = pos1*vertBoneWeights.x+pos2*vertBoneWeights.y+pos3*vertBoneWeights.z+pos4*vertBoneWeights.w;
	pos.w = 1.0;
	gl_Position = matProjViewModel*pos;
}