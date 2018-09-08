//
//  RNVRTrackingState.h
//  Rayne-VR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VRTRACKINGSTATE_H_
#define __RAYNE_VRTRACKINGSTATE_H_

#include "RNVR.h"

namespace RN
{
	struct VRHMDTrackingState
	{
		enum Mode
		{
			Rendering,
			Paused,
			Disconnected
		};

		Vector3 eyeOffset[2];
		Matrix eyeProjection[2];

		Vector3 position;
		Quaternion rotation;

		Mode mode;
	};

	struct VRControllerTrackingState
	{
		enum Type
		{
			Orientation,
			Position
		};

		enum Button
		{
			AX,
			BY,
			Stick,
			Pad,
			Start,
			BUTTON_COUNT
		};

		VRControllerTrackingState() : type(Type::Position), active(false), tracking(false), controllerID(-1), indexTrigger(0.0f), handTrigger(0.0f), button{false, false, false, false, false} {}

		Type type;

		bool active;
		bool tracking;
		uint8 controllerID;

		Vector3 position;
		Quaternion rotation;

		float indexTrigger;
		float handTrigger;
		Vector2 thumbstick;
		Vector2 trackpad;

		bool button[Button::BUTTON_COUNT];
	};

	struct VRControllerHaptics
	{
		VRControllerHaptics() : sampleCount(0) {}
		void Push(uint8 sample)
		{
			samples[sampleCount++] = sample;
		}

		uint8 samples[256];
		uint16 sampleCount;
	};
}


#endif /* __RAYNE_VRTRACKINGSTATE_H_ */
