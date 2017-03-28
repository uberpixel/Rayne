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
#include "RNOculusTrackingState.h"

namespace RN
{
	class OculusSwapChain;
	class OculusWindow : public Window
	{
	public:
/*		enum Eye
		{
			Left,
			Right
		};*/

		OVRAPI OculusWindow();
		OVRAPI ~OculusWindow();

		OVRAPI void SetTitle(const String *title) final { }
		OVRAPI Screen *GetScreen() final { return nullptr; }

		OVRAPI void Show() final { }
		OVRAPI void Hide() final { }

		OVRAPI Vector2 GetSize() const final;

		OVRAPI Framebuffer *GetFramebuffer() const final;

		OVRAPI void UpdateTrackingData(float near, float far);

		OVRAPI const OculusHMDTrackingState &GetHMDTrackingState() const;
		OVRAPI const OculusTouchTrackingState &GetTouchTrackingState(int hand) const;

	private:
		OculusSwapChain *_swapChain;
		OculusHMDTrackingState _hmdTrackingState;
		OculusTouchTrackingState _touchTrackingState[2];

		RNDeclareMetaAPI(OculusWindow, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSWINDOW_H_ */
