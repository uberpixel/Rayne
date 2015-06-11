//
//  RNShader.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SHADER_H_
#define __RAYNE_SHADER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class Shader : public Object
	{
	public:

		RNAPI virtual const String *GetName() const = 0;

		RNDeclareMeta(Shader)
	};
}


#endif /* __RAYNE_SHADER_H_ */
