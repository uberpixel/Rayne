//
//  RND3D12RendererDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12RENDERERDESCRIPTOR_H__
#define __RAYNE_D3D12RENDERERDESCRIPTOR_H__

#include <Rayne.h>

namespace RN
{
	class D3D12RendererDescriptor : public RendererDescriptor
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		D3D12RendererDescriptor();

		Renderer *CreateRenderer(const Dictionary *parameters) override;
		bool CanConstructWithSettings(const Dictionary *parameters) const override;

		RNDeclareMeta(D3D12RendererDescriptor)
	};
}

#endif /* __RAYNE_D3D12RENDERERDESCRIPTOR_H__ */
