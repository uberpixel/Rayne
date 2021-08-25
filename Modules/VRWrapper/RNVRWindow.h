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

#define kRNVRDidRecenter RNCSTR("kRNVRDidRecenter")

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

		enum DeviceType
		{
			OpenVR,
			OculusVR,
			OculusGo,
			OculusQuest,
			OculusQuest2
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

		RNVRAPI virtual void SetFixedFoveatedRenderingLevel(uint8 level, bool dynamic) { }
		RNVRAPI virtual void SetPreferredFramerate(float framerate) { }
		RNVRAPI virtual void SetPerformanceLevel(uint8 cpuLevel, uint8 gpuLevel) { }

		RNVRAPI virtual Vector2 GetSize() const override = 0;

		RNVRAPI virtual Framebuffer *GetFramebuffer() const override = 0;
		RNVRAPI virtual Framebuffer *GetFramebuffer(uint8 eye) const { return nullptr; }

		RNVRAPI virtual void BeginFrame(float delta) {}
		RNVRAPI virtual void Update(float delta, float near, float far) = 0;

		RNVRAPI virtual const VRHMDTrackingState &GetHMDTrackingState() const = 0;
		RNVRAPI virtual const VRControllerTrackingState &GetControllerTrackingState(uint8 index) const = 0;
		RNVRAPI virtual const VRControllerTrackingState &GetTrackerTrackingState(uint8 index) const = 0;
		RNVRAPI virtual const VRHandTrackingState &GetHandTrackingState(uint8 index) const = 0;
		RNVRAPI virtual void SubmitControllerHaptics(uint8 index, VRControllerHaptics &haptics) = 0;
		
		RNVRAPI virtual void PreparePreviewWindow(Window *window) const {}
		RNVRAPI virtual RenderingDevice *GetOutputDevice(RendererDescriptor *descriptor) const = 0;
		
		RNVRAPI virtual Mesh *GetHiddenAreaMesh(uint8 eye) const {return nullptr;}

		RNVRAPI virtual DeviceType GetDeviceType() const = 0;
		RNVRAPI virtual Origin GetOrigin() const { return Origin::Floor; }

		RNVRAPI virtual uint64 GetWindowHandle() const override;

		RNVRAPI virtual Array *GetRequiredVulkanInstanceExtensions() const { return nullptr; };
		RNVRAPI virtual Array *GetRequiredVulkanDeviceExtensions(RN::RendererDescriptor *descriptor, RenderingDevice *device) const { return nullptr; };

		RNDeclareMetaAPI(VRWindow, RNVRAPI)
	};
}


#endif /* __RAYNE_VRWINDOW_H_ */
