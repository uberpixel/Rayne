//
//  RNOculusWindow.h
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSWINDOW_H_
#define __RAYNE_OCULUSWINDOW_H_

#include "RNOculus.h"
#include "RNVRWindow.h"

namespace RN
{
	class OculusSwapChain;
	class OculusWindow : public VRWindow
	{
	public:
/*		enum Eye
		{
			Left,
			Right
		};*/

		OVRAPI OculusWindow();
		OVRAPI ~OculusWindow();

		OVRAPI void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor()) final;
		OVRAPI void StopRendering() final;
		OVRAPI bool IsRendering() const final;

		OVRAPI Vector2 GetSize() const final;
		OVRAPI Framebuffer *GetFramebuffer() const final;
		OVRAPI uint32 GetEyePadding() const final;

		OVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OVRAPI const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const final;
		OVRAPI const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const final;
		OVRAPI void SubmitControllerHaptics(uint8 controllerID, const VRControllerHaptics &haptics) final;

		OVRAPI const String *GetPreferredAudioOutputDeviceID() const;
		OVRAPI const String *GetPreferredAudioInputDeviceID() const;

		OVRAPI RenderingDevice *GetOutputDevice(RendererDescriptor *descriptor) const final;
		OVRAPI const Window::SwapChainDescriptor &GetSwapChainDescriptor() const final;

		OVRAPI void Update(float delta, float near, float far) final;

		OVRAPI static VRWindow::Availability GetAvailability();

	private:
		OculusSwapChain *_swapChain;
		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];
		VRControllerTrackingState _trackerTrackingState;

		RNDeclareMetaAPI(OculusWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSWINDOW_H_ */
