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

uniform mat4 matProj;
uniform mat4 matProjInverse;

#if defined(RN_INSTANCING)

uniform samplerBuffer instancingData;
uniform usamplerBuffer instancingIndices; 

mat4 imatModel(int iOffset)
{
	int index  = int(texelFetch(instancingIndices, gl_InstanceID).r);
	int offset = (index * 8) + iOffset;
	mat4 matrix;

	matrix[0] = texelFetch(instancingData, offset + 0);
	matrix[1] = texelFetch(instancingData, offset + 1);
	matrix[2] = texelFetch(instancingData, offset + 2);
	matrix[3] = texelFetch(instancingData, offset + 3);

	return matrix;
}

#define matModel imatModel(0)
#define matModelInverse imatModel(4)

#define matViewModel (matView * matModel)
#define matViewModelInverse (matViewInverse * matModelInverse)

#define matProjViewModel (matProjView * matModel)
#define matProjViewModelInverse (matProjViewInverse * matModelInverse)

#else

uniform mat4 matModel;
uniform mat4 matModelInverse;

uniform mat4 matViewModel;
uniform mat4 matViewModelInverse;

uniform mat4 matProjViewModel;
uniform mat4 matProjViewModelInverse;

#endif
#endif
