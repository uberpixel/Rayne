//
//  RNOpenXRWindow.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_OpenXRWINDOW_H_
#define __RAYNE_OpenXRWINDOW_H_

#include "RNOpenXR.h"
#include "RNVRWindow.h"

namespace RN
{
	class OpenXRSwapChain;
	class OpenXRVulkanSwapChain;
	class OpenXRD3D12SwapChain;
	struct OpenXRWindowInternals;
	class OpenXRWindow : public VRWindow
	{
	friend OpenXRVulkanSwapChain;
	friend OpenXRD3D12SwapChain;
	public:
		enum SwapChainType
		{
			Metal,
			D3D12,
			Vulkan
		};
		
		OXRAPI OpenXRWindow();
		OXRAPI ~OpenXRWindow();

		OXRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor(), float eyeResolutionFactor = 1.0f) final;
		OXRAPI void StopRendering() final;
		OXRAPI bool IsRendering() const final;

		OXRAPI void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) final;
		OXRAPI void SetPreferredFramerate(float framerate) final;
		OXRAPI void SetPerformanceLevel(uint8 cpuLevel, uint8 gpuLevel) final;
		OXRAPI void SetLocalDimming(bool enabled) final;

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
		void InitializeInput();
		const String *GetHMDInfoDescription() const;

		int _mainThreadID;
		OpenXRWindowInternals *_internals;
		DeviceType _deviceType;

		SwapChainType _swapChainType;
		OpenXRSwapChain *_swapChain;
		uint32 _actualFrameIndex;
        double _predictedDisplayTime;

		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];
		VRControllerTrackingState _trackerTrackingState;
		VRHandTrackingState _handTrackingState[2];

		uint8 _currentHapticsIndex[2];
		VRControllerHaptics _haptics[2];
		bool _hapticsStopped[2];

		float _preferredFrameRate;
		uint8 _minCPULevel;
		uint8 _minGPULevel;
		uint8 _fixedFoveatedRenderingLevel;
		bool _fixedFoveatedRenderingDynamic;
		bool _isLocalDimmingEnabled;

		bool _isSessionRunning;
		bool _hasSynchronization;
		bool _hasVisibility;
		bool _hasInputFocus;

		bool _supportsD3D12;
		bool _supportsVulkan;

		bool _supportsPerformanceLevels;
		bool _supportsPreferredFramerate;
		bool _supportsAndroidThreadType;
		bool _supportsFoveatedRendering;
		bool _supportsLocalDimming;

#if RN_OPENXR_SUPPORTS_PICO_LOADER
		int _gsIndexPICO;
		bool _supportsViewStatePICO;
		bool _supportsFrameEndInfoPICO;
		bool _supportsSessionBeginInfoPICO;
		bool _supportsAndroidControllerFunctionPICO;
		bool _supportsConfigsPICO;
#endif

		RNDeclareMetaAPI(OpenXRWindow, OXRAPI)
	};
}


#endif /* __RAYNE_OpenXRWINDOW_H_ */
