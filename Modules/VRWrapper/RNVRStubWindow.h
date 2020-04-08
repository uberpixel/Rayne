//
//  RNVRStubWindow.h
//  Rayne-VR
//
//  Copyright 2020 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_VRSTUBWINDOW_H_
#define __RAYNE_VRSTUBWINDOW_H_

#include "RNVR.h"
#include "RNVRWindow.h"

namespace RN
{
	class VRStubWindow : public VRWindow
	{
	public:
		RNVRAPI VRStubWindow() { }
		RNVRAPI ~VRStubWindow() { }
		
		RNVRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor()) final { }
		RNVRAPI void StopRendering() final { }
		RNVRAPI bool IsRendering() const final { return true; }

		RNVRAPI Vector2 GetSize() const final { return Vector2(); }

		RNVRAPI Framebuffer *GetFramebuffer() const final { return nullptr; }
		RNVRAPI Framebuffer *GetFramebuffer(uint8 eye) const final { return nullptr; }
		RNVRAPI uint32 GetEyePadding() const final { return 0; }

		RNVRAPI void Update(float delta, float near, float far) final {}

		RNVRAPI const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final { return Window::SwapChainDescriptor(); }

		RNVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final { return VRHMDTrackingState(); }
		RNVRAPI const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const final { return VRControllerTrackingState(); }
		RNVRAPI const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const final { return VRControllerTrackingState(); }
		RNVRAPI void SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics) final {}
		
		RNVRAPI RenderingDevice *GetOutputDevice(RendererDescriptor *descriptor) const final { return nullptr; }

		RNDeclareMetaAPI(VRStubWindow, RNVRAPI)
	};

	RNDefineMeta(VRStubWindow, VRWindow)
}


#endif /* __RAYNE_VRSTUBWINDOW_H_ */
