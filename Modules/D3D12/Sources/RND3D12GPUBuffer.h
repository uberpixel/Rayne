//
//  RND3D12GPUBuffer.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_D3D12GPUBUFFER_H_
#define __RAYNE_D3D12GPUBUFFER_H_

#include <Rayne.h>

namespace RN
{
	class D3D12Renderer;
	class D3D12GPUBuffer : public GPUBuffer
	{
	public:
		friend class D3D12Renderer;

		RNAPI void *GetBuffer() const final;
		RNAPI void InvalidateRange(const Range &range) final;
		RNAPI size_t GetLength() const final;

	private:
		D3D12GPUBuffer(void *data);
		~D3D12GPUBuffer() override;

		void *_buffer;

		RNDeclareMeta(D3D12GPUBuffer)
	};
}


#endif /* __RAYNE_D3D12GPUBUFFER_H_ */
