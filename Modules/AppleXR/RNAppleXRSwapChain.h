//
//  RNAppleXRSwapChain.h
//  Rayne-AppleXR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_APPLEXRSWAPCHAIN_H_
#define __RAYNE_APPLEXRSWAPCHAIN_H_

#include "RNAppleXR.h"
#include <CompositorServices/CompositorServices.h>

namespace RN
{
	class AppleXRSwapChain
	{
	public:
		friend class AppleXRWindow;

		AXRAPI ~AppleXRSwapChain();

		AXRAPI virtual void UpdatePredictedPose();

		AXRAPI virtual Vector2 GetAppleXRSwapChainSize() const = 0;
		AXRAPI virtual const Window::SwapChainDescriptor &GetAppleXRSwapChainDescriptor() const = 0;
		AXRAPI virtual Framebuffer *GetAppleXRSwapChainFramebuffer() const = 0;
		
		bool isActive;

	protected:
		AppleXRSwapChain(cp_layer_renderer_t layerRenderer);
		
		cp_layer_renderer_t _layerRenderer;
		ar_device_anchor_t _worldAnchor;
		
		cp_frame_t _frame;
		cp_frame_timing_t _predictedTime;
		
		Vector3 _hmdToEyeViewOffset[2];
		Matrix _hmdEyeProjectionMatrix[2];
	};
}


#endif /* __RAYNE_APPLEXRSWAPCHAIN_H_ */
