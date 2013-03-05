//
//  rn_Matrices.vsh
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

uniform mat4 matProjView;
uniform mat4 matProjViewInverse;

uniform mat4 matView;
uniform mat4 matViewInverse;

#ifdef RN_INSTANCING

in mat4 imatModel;
in mat4 imatModelInverse;

#define matModel imatModel
#define matModelInverse imatModelInverse

#define matViewModel (matView * imatModel)
#define matViewModelInverse (matViewInverse * imatModelInverse)

#define matProjViewModel (matProjView * imatModel)
#define matProjViewModelInverse (matProjViewInverse * imatModelInverse)

#else

uniform mat4 matModel;
uniform mat4 matModelInverse;

uniform mat4 matProjViewModel;
uniform mat4 matProjViewModelInverse;

#endif
