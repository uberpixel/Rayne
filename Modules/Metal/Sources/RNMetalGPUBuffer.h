//
//  RNMetalGPUBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALGPUBUFFER_H_
#define __RAYNE_METALGPUBUFFER_H_

#include <Rayne.h>

namespace RN
{
	class MetalRenderer;
	class MetalGPUBuffer : public GPUBuffer
	{
	public:
		friend class MetalRenderer;

		RNAPI void *GetBuffer() const final;
		RNAPI void InvalidateRange(const Range &range) final;
		RNAPI size_t GetLength() const final;

	private:
		MetalGPUBuffer(void *data);
		~MetalGPUBuffer() override;

		void *_buffer;

		RNDeclareMeta(MetalGPUBuffer)
	};
}


#endif /* __RAYNE_METALGPUBUFFER_H_ */
