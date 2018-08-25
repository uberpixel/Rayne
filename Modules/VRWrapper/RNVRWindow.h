//
//  RNVRWindow.h
//  Rayne-VR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VRWINDOW_H_
#define __RAYNE_VRWINDOW_H_

#include "RNVR.h"
#include "RNVRTrackingState.h"

namespace RN
{
	class VRSwapChain;
	class VRWindow : public Window
	{
	public:
/*		enum Eye
		{
			Left,
			Right
		};*/

		enum Origin
		{
			Floor,
			Head
		};

		enum Availability
		{
			None,
			Software,
			HMD
		};

		RNVRAPI VRWindow();
		RNVRAPI ~VRWindow();
		
		RNVRAPI virtual void StartRendering(const SwapChainDescriptor &descriptor = SwapChainDescriptor()) = 0;
		RNVRAPI virtual void StopRendering() = 0;
		RNVRAPI virtual bool IsRendering() const = 0;

		RNVRAPI virtual void SetTitle(const String *title) override { }
		RNVRAPI virtual Screen *GetScreen() override { return nullptr; }

		RNVRAPI virtual void Show() override { }
		RNVRAPI virtual void Hide() override { }
		RNVRAPI virtual void SetFullscreen(bool fullscreen) override {}

		RNVRAPI virtual Vector2 GetSize() const override = 0;

		RNVRAPI virtual Framebuffer *GetFramebuffer() const override = 0;
		RNVRAPI virtual uint32 GetEyePadding() const = 0;

		RNVRAPI virtual void Update(float delta, float near, float far) = 0;

		RNVRAPI virtual const VRHMDTrackingState &GetHMDTrackingState() const = 0;
		RNVRAPI virtual const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const = 0;
		RNVRAPI virtual const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const = 0;
		RNVRAPI virtual void SubmitControllerHaptics(uint8 controllerID, const VRControllerHaptics &haptics) = 0;
		
		RNVRAPI virtual void PreparePreviewWindow(Window *window) const {}
		RNVRAPI virtual RenderingDevice *GetOutputDevice() const = 0;
		
		RNVRAPI virtual Mesh *GetHiddenAreaMesh(uint8 eye) const {return nullptr;}

		RNVRAPI virtual Origin GetOrigin() const { return Origin::Floor; }

		RNVRAPI virtual uint64 GetWindowHandle() const override;

		RNDeclareMetaAPI(VRWindow, RNVRAPI)
	};
}


#endif /* __RAYNE_VRWINDOW_H_ */
