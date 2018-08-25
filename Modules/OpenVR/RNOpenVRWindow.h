//
//  RNOpenVRWindow.h
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVRWINDOW_H_
#define __RAYNE_OPENVRWINDOW_H_

#include "RNVRWindow.h"
#include "RNOpenVR.h"
#include "RNVRTrackingState.h"

namespace vr
{
	class IVRSystem;
}

namespace RN
{
	class OpenVRSwapChain;

	class OpenVRWindow : public VRWindow
	{
	public:
		enum SwapChainType
		{
			Metal,
			D3D12,
			Vulkan
		};

		OVRAPI OpenVRWindow();
		OVRAPI ~OpenVRWindow();

		OVRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor()) final;
		OVRAPI void StopRendering() final;
		OVRAPI bool IsRendering() const final;

		OVRAPI Vector2 GetSize() const final;

		OVRAPI Framebuffer *GetFramebuffer() const final;
		OVRAPI uint32 GetEyePadding() const final;

		OVRAPI void Update(float delta, float near, float far) final;

		OVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OVRAPI const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const final;
		OVRAPI const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const final;
		OVRAPI void SubmitControllerHaptics(uint8 controllerID, const VRControllerHaptics &haptics) final;

		OVRAPI void UpdateSize(const Vector2 &size);

		OVRAPI void PreparePreviewWindow(Window *window) const final;
		OVRAPI RenderingDevice *GetOutputDevice() const final;

		OVRAPI Mesh *GetHiddenAreaMesh(uint8 eye) const final;
		OVRAPI const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final;

		OVRAPI static VRWindow::Availability GetAvailability();
		OVRAPI static bool IsSteamVRRunning();

	private:
		const String *GetHMDInfoDescription() const;

		OpenVRSwapChain *_swapChain;
		SwapChainType _swapChainType;

		vr::IVRSystem *_vrSystem;
		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];
		VRControllerTrackingState _trackerTrackingState;

		VRControllerHaptics _haptics[3];
		uint16 _currentHapticsIndex[3];
		float _remainingHapticsDelta;

		float _lastSizeChangeTimer;
		Vector2 _lastSize;

		RNDeclareMetaAPI(OpenVRWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRWINDOW_H_ */
