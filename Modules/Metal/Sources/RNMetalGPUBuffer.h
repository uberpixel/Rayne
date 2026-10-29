//
//  RNMetalGPUBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_METALGPUBUFFER_H_
#define __RAYNE_METALGPUBUFFER_H_

#include "RNMetal.h"

namespace RN
{
	class MetalRenderer;
	class MetalDynamicGPUBuffer;
	class MetalGPUBuffer : public GPUBuffer
	{
	public:
		friend class MetalRenderer;
		friend class MetalDynamicGPUBuffer;

		MTLAPI void *GetBuffer() override;
		MTLAPI void UnmapBuffer() override;
		MTLAPI void InvalidateRange(const Range &range) override;
		MTLAPI void FlushRange(const Range &range, bool discard = false) override;
		MTLAPI size_t GetLength() const override;
		
	protected:
		MetalGPUBuffer(void *data);
		~MetalGPUBuffer() override;

		void *_buffer; //The active buffer used for rendering

	private:
		RNDeclareMetaAPI(MetalGPUBuffer, MTLAPI)
	};
}


#endif /* __RAYNE_METALGPUBUFFER_H_ */
