//
//  RNOculusTrackingState.h
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUSTRACKINGSTATE_H_
#define __RAYNE_OCULUSTRACKINGSTATE_H_

#include "RNOculus.h"

namespace RN
{
	struct OculusHMDTrackingState
	{
		Vector3 eyeOffset[2];
		Matrix eyeProjection[2];

		Vector3 position;
		Quaternion rotation;
	};

	struct OculusTouchTrackingState
	{
		bool active;

		Vector3 position;
		Quaternion rotation;

		Vector2 thumbstick;
	};
}


#endif /* __RAYNE_OCULUSTRACKINGSTATE_H_ */
