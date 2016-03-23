//
//  RNGPUResource.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_GPURESOURCE_H_
#define __RAYNE_GPURESOURCE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class GPUResource : public Object
	{
	public:
		enum class UsageOptions
		{
			Uniform,
			Vertex,
			Index
		};

		enum class AccessOptions
		{
			ReadWrite,
			WriteOnly,
			Private
		};

		RNAPI ~GPUResource();

	protected:
		RNAPI GPUResource();

		__RNDeclareMetaInternal(GPUResource)
	};
}


#endif /* __RAYNE_GPURESOURCE_H_ */
