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

namespace RN
{
	class OculusWindow;
	class OculusCamera : public SceneNode
	{
	public:
		OVRAPI OculusCamera(bool debug = false);
		OVRAPI ~OculusCamera();

		void Update(float delta) override;

	private:
		OculusWindow *_window;
		Camera *_camera[2];

		bool _isDebug;
		Window *_debugWindow;

		RNDeclareMetaAPI(OculusCamera, OVRAPI)
	};
}


#endif /* __RAYNE_OCULUSCAMERA_H_ */
