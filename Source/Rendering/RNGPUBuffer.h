//
//  RNGPUBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_GPUBUFFER_H_
#define __RAYNE_GPUBUFFER_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class GPUBuffer : public Object
	{
	public:
		enum class Options
		{
			ReadWrite,
			WriteOnly,
			Private
		};

		RNAPI virtual void *GetBuffer() const = 0;
		RNAPI virtual void InvalidateRange(const Range &range) = 0;
		RNAPI virtual size_t GetLength() const = 0;

		RNDeclareMeta(GPUBuffer)
	};
}


#endif /* __RAYNE_GPUBUFFER_H_ */
