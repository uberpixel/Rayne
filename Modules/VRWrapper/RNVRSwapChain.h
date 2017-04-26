//
//  RNVRSwapChain.h
//  Rayne-VR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VRSWAPCHAIN_H_
#define __RAYNE_VRSWAPCHAIN_H_

#include "RND3D12Renderer.h"
#include "RND3D12SwapChain.h"
#include "RNVR.h"

namespace RN
{
	class VRSwapChain : public D3D12SwapChain
	{
	public:
		RNVRAPI ~VRSwapChain();

		RNVRAPI virtual void AcquireBackBuffer() override = 0;
		RNVRAPI virtual void Prepare(D3D12CommandList *commandList) override = 0;
		RNVRAPI virtual void Finalize(D3D12CommandList *commandList) override = 0;
		RNVRAPI virtual void PresentBackBuffer() override = 0;

		RNVRAPI virtual ID3D12Resource *GetD3D12Buffer(int i) const override = 0;

		RNVRAPI virtual void UpdatePredictedPose() = 0;

	protected:
		RNVRAPI VRSwapChain();

		RNDeclareMetaAPI(VRSwapChain, RNVRAPI)
	};
}


#endif /* __RAYNE_VRSWAPCHAIN_H_ */
