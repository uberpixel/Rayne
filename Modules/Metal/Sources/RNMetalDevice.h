//
//  RNMetalDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALDEVICE_H_
#define __RAYNE_METALDEVICE_H_

#include "RNMetal.h"

namespace RN
{
	class MetalDevice : public RenderingDevice
	{
	public:
		friend class MetalRendererDescriptor;

		id<MTLDevice> GetDevice() const { return _device; }

	private:
		static Descriptor DescriptorFromDevice(id<MTLDevice> device);

		MetalDevice(id<MTLDevice> device);

		id<MTLDevice> _device;

		RNDeclareMetaAPI(MetalDevice, MTLAPI)
	};
}


#endif /* __RAYNE_METALDEVICE_H_ */
