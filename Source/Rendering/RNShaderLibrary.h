//
//  RNShaderLibrary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SHADERLIBRARY_H_
#define __RAYNE_SHADERLIBRARY_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "RNShader.h"

namespace RN
{
	class ShaderLibrary : public Object
	{
	public:
		RNAPI virtual Shader *GetShaderWithName(const String *name) = 0;
		RNAPI virtual Array *GetShaderNames() const = 0;

		RNDeclareMeta(ShaderLibrary)
	};
}


#endif /* __RAYNE_SHADERLIBRARY_H_ */
