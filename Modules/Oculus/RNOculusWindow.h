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

		OVRAPI Vector2 GetSize() const final;
		OVRAPI Framebuffer *GetFramebuffer() const final;

		OVRAPI void UpdateTrackingData(float near, float far) final;

		OVRAPI const VRHMDTrackingState &GetHMDTrackingState() const final;
		OVRAPI const VRControllerTrackingState &GetControllerTrackingState(int hand) const final;
		OVRAPI void SubmitControllerHaptics(int hand, const VRControllerHaptics &haptics) final;

	private:
		OculusSwapChain *_swapChain;
		VRHMDTrackingState _hmdTrackingState;
		VRControllerTrackingState _controllerTrackingState[2];

		RNDeclareMetaAPI(OculusWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSWINDOW_H_ */
