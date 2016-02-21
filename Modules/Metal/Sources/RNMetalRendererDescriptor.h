//
//  RNMetalRendererDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERERDESCRIPTOR_H__
#define __RAYNE_METALRENDERERDESCRIPTOR_H__

#include "RNMetal.h"

namespace RN
{
	class MetalRendererDescriptor : public RendererDescriptor
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		MTLAPI MetalRendererDescriptor();

		MTLAPI Renderer *CreateRenderer(RenderingDevice *device) override;
		MTLAPI bool CanCreateRenderer() const override;

		MTLAPI void PrepareWithSettings(const Dictionary *settings) override;

		const Array *GetDevices() const override { return _devices; }

	private:
		Array *_devices;

		RNDeclareMetaAPI(MetalRendererDescriptor, MTLAPI)
	};
}

#endif /* __RAYNE_METALRENDERERDESCRIPTOR_H__ */
