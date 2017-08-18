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

namespace RN
{
#if RN_PLATFORM_MAC_OS
	class OpenVRMetalSwapChain;
#elif RN_PLATFORM_WINDOWS
	class OpenVRD3D12SwapChain;
#endif
	class OpenVRWindow : public VRWindow
	{
	public:
/*		enum Eye
		{
			Left,
			Right
		};*/

		OVRAPI OpenVRWindow(const SwapChainDescriptor &descriptor = SwapChainDescriptor());
		OVRAPI ~OpenVRWindow();

		OVRAPI Vector2 GetSize() const final;

		OVRAPI Framebuffer *GetFramebuffer() const final;
		OVRAPI uint32 GetEyePadding() const final;

		OVRAPI void Update(float delta, float near, float far) final;

		OVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OVRAPI const VRControllerTrackingState &GetControllerTrackingState(int hand) const final;
		OVRAPI void SubmitControllerHaptics(int hand, const VRControllerHaptics &haptics) final;

		OVRAPI void UpdateSize(const Vector2 &size);

	private:
#if RN_PLATFORM_MAC_OS
		OpenVRMetalSwapChain *_swapChain;
#elif RN_PLATFORM_WINDOWS
		OpenVRD3D12SwapChain *_swapChain;
#endif
		
		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];

		VRControllerHaptics _haptics[2];
		uint16 _currentHapticsIndex[2];
		float _remainingHapticsDelta;

		float _lastSizeChangeTimer;
		Vector2 _lastSize;

		RNDeclareMetaAPI(OpenVRWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRWINDOW_H_ */
