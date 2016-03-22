//
//  RND3D12RendererDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12RENDERERDESCRIPTOR_H__
#define __RAYNE_D3D12RENDERERDESCRIPTOR_H__

#include "RND3D12.h"

namespace RN
{
	class D3D12RendererDescriptor : public RendererDescriptor
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		D3DAPI D3D12RendererDescriptor();

		D3DAPI Renderer *CreateRenderer(RenderingDevice *device) final;
		D3DAPI bool CanCreateRenderer() const final;

		D3DAPI void PrepareWithSettings(const Dictionary *settings) final;

		D3DAPI const Array *GetDevices() const final;

	private:
		IDXGIFactory4 *_factory;
		Array *_devices;

		RNDeclareMeta(D3D12RendererDescriptor)
	};
}

#endif /* __RAYNE_D3D12RENDERERDESCRIPTOR_H__ */
