//
//  RNMetalRendererDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERERDESCRIPTOR_H__
#define __RAYNE_METALRENDERERDESCRIPTOR_H__

#include <Rayne.h>

namespace RN
{
	class MetalRendererDescriptor : public RendererDescriptor
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		MetalRendererDescriptor();

		Renderer *CreateRenderer(RenderingDevice *device) override;
		bool CanCreateRenderer() const override;

		void PrepareWithSettings(const Dictionary *settings) override;

		const Array *GetDevices() const override { return _devices; }

	private:
		Array *_devices;

		RNDeclareMeta(MetalRendererDescriptor)
	};
}

#endif /* __RAYNE_METALRENDERERDESCRIPTOR_H__ */
