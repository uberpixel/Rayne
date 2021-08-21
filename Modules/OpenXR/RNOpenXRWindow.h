//
//  RNOpenXRWindow.h
//  Rayne-OpenXR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OpenXRWINDOW_H_
#define __RAYNE_OpenXRWINDOW_H_

#include "RNOpenXR.h"
#include "RNVRWindow.h"

namespace RN
{
	class OpenXRVulkanSwapChain;
	class OpenXRWindowInternals;
	class OpenXRWindow : public VRWindow
	{
	friend OpenXRVulkanSwapChain;
	public:
		OXRAPI OpenXRWindow();
		OXRAPI ~OpenXRWindow();

		OXRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor()) final;
		OXRAPI void StopRendering() final;
		OXRAPI bool IsRendering() const final;

		OXRAPI void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) final;
		OXRAPI void SetPreferredFramerate(uint32 framerate) final;
		OXRAPI void SetPerformanceLevel(uint8 cpuLevel, uint8 gpuLevel) final;

		OXRAPI Vector2 GetSize() const final;
		OXRAPI Framebuffer *GetFramebuffer() const final;
		OXRAPI Framebuffer *GetFramebuffer(uint8 eye) const final;

		OXRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OXRAPI const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const final;
		OXRAPI const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const final;
		OXRAPI const VRHandTrackingState &GetHandTrackingState(uint8 index) const final;
		OXRAPI void SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics) final;

		OXRAPI const String *GetPreferredAudioOutputDeviceID() const;
		OXRAPI const String *GetPreferredAudioInputDeviceID() const;

		OXRAPI RenderingDevice *GetOutputDevice(RendererDescriptor *descriptor) const final;
		OXRAPI const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final;

		OXRAPI void BeginFrame(float delta) final;
		OXRAPI void Update(float delta, float near, float far) final;

		OXRAPI VRWindow::DeviceType GetDeviceType() const final;
		OXRAPI VRWindow::Origin GetOrigin() const final { return VRWindow::Origin::Floor; }

		OXRAPI Array *GetRequiredVulkanInstanceExtensions() const final;
        OXRAPI Array *GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *device) const final;

	private:
		const String *GetHMDInfoDescription() const;
		void UpdateVRMode();

		int _mainThreadID;
		OpenXRWindowInternals *_internals;
		void *_session;
		void *_nativeWindow;

		OpenXRVulkanSwapChain *_swapChain;
		uint32 _actualFrameIndex;
        double _predictedDisplayTime;

		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];
		VRControllerTrackingState _trackerTrackingState;
		VRHandTrackingState _handTrackingState[2];

		uint8 _currentHapticsIndex[2];
		VRControllerHaptics _haptics[2];
		bool _hapticsStopped[2];

		uint32 _preferredFrameRate;
		uint8 _minCPULevel;
		uint8 _minGPULevel;
		uint8 _fixedFoveatedRenderingLevel;
		bool _fixedFoveatedRenderingDynamic;

		bool _hasInputFocus;
		bool _hasVisibility;

		RNDeclareMetaAPI(OpenXRWindow, OXRAPI)
	};
}


#endif /* __RAYNE_OpenXRWINDOW_H_ */
