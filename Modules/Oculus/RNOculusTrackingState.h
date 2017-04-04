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
		enum Button
		{
			AX,
			BY,
			Thumb,
			Enter,
			BUTTON_COUNT
		};

		OculusTouchTrackingState() : button{false, false, false, false} {}

		bool active;

		Vector3 position;
		Quaternion rotation;

		float indexTrigger;
		float handTrigger;
		Vector2 thumbstick;

		bool button[Button::BUTTON_COUNT];
	};

	struct OculusTouchHaptics
	{
		OculusTouchHaptics() : sampleCount(0) {}
		void Push(uint8 sample)
		{
			samples[sampleCount++] = sample;
		}

		uint8 samples[256];
		uint16 sampleCount;
	};
}


#endif /* __RAYNE_OCULUSTRACKINGSTATE_H_ */
