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
	class OpenVRSwapChain;
	class OpenVRWindow : public VRWindow
	{
	public:
/*		enum Eye
		{
			Left,
			Right
		};*/

		OVRAPI OpenVRWindow();
		OVRAPI ~OpenVRWindow();

		OVRAPI Vector2 GetSize() const final;

		OVRAPI Framebuffer *GetFramebuffer() const final;

		OVRAPI void Update(float delta, float near, float far) final;

		OVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OVRAPI const VRControllerTrackingState &GetControllerTrackingState(int hand) const final;
		OVRAPI void SubmitControllerHaptics(int hand, const VRControllerHaptics &haptics) final;

	private:
		OpenVRSwapChain *_swapChain;
		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];

		VRControllerHaptics _haptics[2];
		uint16 _currentHapticsIndex[2];
		float _remainingHapticsDelta;

		RNDeclareMetaAPI(OpenVRWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OPENVRWINDOW_H_ */
