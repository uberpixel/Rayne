//
//  RNOculusCamera.h
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSCAMERA_H_
#define __RAYNE_OCULUSCAMERA_H_

#include "RNOculus.h"
#include "RNOculusTrackingState.h"
#include "RNOculusWindow.h"

namespace RN
{
	class OculusWindow;
	class OculusCamera : public SceneNode
	{
	public:
		OVRAPI OculusCamera(bool debug = false);
		OVRAPI ~OculusCamera();

		OVRAPI virtual void Update(float delta) override;

		OVRAPI void ReCenter();

		SceneNode *GetHead() const { return _head; }

		OVRAPI const OculusHMDTrackingState &GetHMDTrackingState();
		OVRAPI const OculusTouchTrackingState &GetTouchTrackingState(int hand);
		OVRAPI void SubmitTouchHaptics(int hand, const OculusTouchHaptics &haptics);

	private:
		OculusWindow *_window;
		SceneNode *_head;
		Camera *_eye[2];

		bool _isDebug;
		Window *_debugWindow;

		RNDeclareMetaAPI(OculusCamera, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSCAMERA_H_ */
