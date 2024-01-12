//
//  RNAppleXRWindow.h
//  Rayne-AppleXR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_APPLEXRWINDOW_H_
#define __RAYNE_APPLEXRWINDOW_H_

#include "RNVRWindow.h"
#include "RNAppleXR.h"
#include "RNVRTrackingState.h"

#include <CompositorServices/CompositorServices.h>

namespace RN
{
	class AppleXRSwapChain;

	class AppleXRWindow : public VRWindow
	{
	public:
		enum SwapChainType
		{
			Metal
		};

		AXRAPI AppleXRWindow();
		AXRAPI ~AppleXRWindow();

		AXRAPI void BeginFrame(float delta);
		AXRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor(), float eyeResolutionFactor = 1.0f) final;
		AXRAPI void StopRendering() final;
		AXRAPI bool IsRendering() const final;

		AXRAPI Vector2 GetSize() const final;

		AXRAPI Framebuffer *GetFramebuffer() const final;

		AXRAPI void Update(float delta, float near, float far) final;

		AXRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		AXRAPI const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const final;
		AXRAPI const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const final;
		AXRAPI const VRHandTrackingState &GetHandTrackingState(uint8 index) const final;
		AXRAPI void SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics) final;

		AXRAPI RenderingDevice *GetOutputDevice(RendererDescriptor *descriptor) const final;

		AXRAPI const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final;
		
		AXRAPI VRWindow::DeviceType GetDeviceType() const final { return VRWindow::DeviceType::AppleXR; }
		AXRAPI String *GetRuntimeName() const final { return RNCSTR("visionOS");  }

	private:
		const String *GetHMDInfoDescription() const;

		AppleXRSwapChain *_swapChain;
		SwapChainType _swapChainType;
		cp_layer_renderer_t _layerRenderer;
		ar_session_t _arSession;
		ar_world_tracking_provider_t _worldTrackingProvider;
		
		bool _isSessionRunning;
		
		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];
		VRControllerTrackingState _trackerTrackingState;
		VRHandTrackingState _handTrackingState[2];

		uint64_t _inputActionSetHandle;
		uint64_t _inputActionHandle[18];

		VRControllerHaptics _haptics[2];
		uint16 _currentHapticsIndex[2];

		RNDeclareMetaAPI(AppleXRWindow, AXRAPI)
	};
}


#endif /* __RAYNE_APPLEXRWINDOW_H_ */
