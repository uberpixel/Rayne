//
//  rn_Matrices.vsh
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef RN_MATRICES_VSH
#define RN_MATRICES_VSH

uniform mat4 matProjView;
uniform mat4 matProjViewInverse;

uniform mat4 matView;
uniform mat4 matViewInverse;

#ifdef RN_INSTANCING

uniform sampler2D instancingData;

mat4 imatModel()
{
	int offset = gl_InstanceID * 4;
	mat4 matrix;

	matrix[0] = texelFetch(instancingData, ivec2(offset + 0, 0), 0);
	matrix[1] = texelFetch(instancingData, ivec2(offset + 1, 0), 0);
	matrix[2] = texelFetch(instancingData, ivec2(offset + 2, 0), 0);
	matrix[3] = texelFetch(instancingData, ivec2(offset + 3, 0), 0);

	return matrix;
}

mat4 imatModelInverse()
{
	int offset = gl_InstanceID * 4;
	mat4 matrix;

	matrix[0] = texelFetch(instancingData, ivec2(offset + 0, 1), 0);
	matrix[1] = texelFetch(instancingData, ivec2(offset + 1, 1), 0);
	matrix[2] = texelFetch(instancingData, ivec2(offset + 2, 1), 0);
	matrix[3] = texelFetch(instancingData, ivec2(offset + 3, 1), 0);

	return matrix;
}

#define matModel imatModel()
#define matModelInverse imatModelInverse()

#define matViewModel (matView * matModel)
#define matViewModelInverse (matViewInverse * matModelInverse)

#define matProjViewModel (matProjView * matModel)
#define matProjViewModelInverse (matProjViewInverse * matModelInverse)

#else

uniform mat4 matModel;
uniform mat4 matModelInverse;

uniform mat4 matProjViewModel;
uniform mat4 matProjViewModelInverse;

#endif
#endif
