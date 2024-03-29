//
//  RNOculusMobileWindow.h
//  Rayne-OculusMobile
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSMOBILEWINDOW_H_
#define __RAYNE_OCULUSMOBILEWINDOW_H_

#include "RNOculusMobile.h"
#include "RNVRWindow.h"



namespace RN
{
	class OculusMobileVulkanSwapChain;
	class OculusMobileWindow : public VRWindow
	{
	public:
		OVRAPI OculusMobileWindow();
		OVRAPI ~OculusMobileWindow();

		OVRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor(), float eyeResolutionFactor = 1.0f) final;
		OVRAPI void StopRendering() final;
		OVRAPI bool IsRendering() const final;

		OVRAPI void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) final;
		OVRAPI void SetPreferredFramerate(uint32 framerate) final;
		OVRAPI void SetPerformanceLevel(uint8 cpuLevel, uint8 gpuLevel) final;

		OVRAPI Vector2 GetSize() const final;
		OVRAPI Framebuffer *GetFramebuffer() const final;
		OVRAPI Framebuffer *GetFramebuffer(uint8 eye) const final;

		OVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OVRAPI const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const final;
		OVRAPI const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const final;
		OVRAPI const VRHandTrackingState &GetHandTrackingState(uint8 index) const final;
		OVRAPI void SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics) final;

		OVRAPI const String *GetPreferredAudioOutputDeviceID() const;
		OVRAPI const String *GetPreferredAudioInputDeviceID() const;

		OVRAPI RenderingDevice *GetOutputDevice(RendererDescriptor *descriptor) const final;
		OVRAPI const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final;

		OVRAPI void BeginFrame(float delta) final;
		OVRAPI void Update(float delta, float near, float far) final;

		OVRAPI VRWindow::DeviceType GetDeviceType() const final;
		OVRAPI VRWindow::Origin GetOrigin() const final { return VRWindow::Origin::Floor; }

		OVRAPI Array *GetRequiredVulkanInstanceExtensions() const final;
        OVRAPI Array *GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *device) const final;

	private:
		const String *GetHMDInfoDescription() const;
		void UpdateVRMode();

		int _mainThreadID;
		void *_java;
		void *_session;
		void *_nativeWindow;

		OculusMobileVulkanSwapChain *_swapChain;
		uint32 _actualFrameIndex;
        double _predictedDisplayTime;

		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];
		VRControllerTrackingState _trackerTrackingState;
		VRHandTrackingState _handTrackingState[2];

		uint8 _currentHapticsIndex[2];
		VRControllerHaptics _haptics[2];

		uint32 _preferredFrameRate;
		uint8 _minCPULevel;
		uint8 _minGPULevel;
		uint8 _fixedFoveatedRenderingLevel;
		bool _fixedFoveatedRenderingDynamic;

		bool _hasInputFocus;
		bool _hasVisibility;

		RNDeclareMetaAPI(OculusMobileWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSMOBILEWINDOW_H_ */
