//
//  RND3D12Device.h
//  Rayne-D3D12
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12DEVICE_H__
#define __RAYNE_D3D12DEVICE_H__

#include "RND3D12.h"

namespace RN
{
	class D3D12Device : public RenderingDevice
	{
	public:
		D3DAPI D3D12Device(IDXGIAdapter1 *adapter);

	private:
		IDXGIAdapter1 *_adapter;

		RNDeclareMeta(D3D12Device)
	};
}

#endif /* __RAYNE_D3D12DEVICE_H__ */
