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
#include "RNGPUResource.h"

namespace RN
{
	class GPUBuffer : public GPUResource
	{
	public:
		RNAPI ~GPUBuffer();
		RNAPI virtual void *GetBuffer() = 0;
		RNAPI virtual void Invalidate();
		RNAPI virtual void InvalidateRange(const Range &range) = 0;
		RNAPI virtual size_t GetLength() const = 0;

	protected:
		RNAPI GPUBuffer();

		__RNDeclareMetaInternal(GPUBuffer)
	};
}


#endif /* __RAYNE_GPUBUFFER_H_ */
