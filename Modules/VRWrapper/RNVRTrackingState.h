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
		enum Type
		{
			ThreeDegreesOfFreedom,
			SixDegreesOfFreedom
		};

		enum Mode
		{
			Rendering,
			Paused,
			Disconnected
		};

		VRHMDTrackingState() : type(Type::SixDegreesOfFreedom), mode(Paused) {}

		Type type;

		Vector3 eyeOffset[2];
		Matrix eyeProjection[2];

		Vector3 position;
		Quaternion rotation;

		Mode mode;
	};

	struct VRHandTrackingState
	{
		VRHandTrackingState() : pinchStrength{0.0f, 0.0f, 0.0f, 0.0f}, active(false), tracking(false), confidence(0), menuButton(false) {}

		enum PinchFinger
		{
			PinchFingerIndex,
			PinchFingerMiddle,
			PinchFingerRing,
			PinchFingerPinky
		};

		float pinchStrength[4];

		bool active;
		bool tracking;
		uint8 confidence;

		bool menuButton;

		Vector3 position;
		Quaternion rotation;
	};

	struct VRControllerTrackingState
	{
		enum Type
		{
			None,
			KhronosSimpleController,
			OculusTouchController,
			HTCViveController,
			ValveIndexController,
			MicrosoftMixedRealityController,
			PicoNeo3Controller
		};

		enum Button
		{
			AX,
			BY,
			Stick,
			Pad,
			PadTouched,
			Start,
			BUTTON_COUNT
		};

		VRControllerTrackingState() : type(Type::None), hasHaptics(true), hapticsSampleLength(0.0), hapticsMaxSamples(0), active(false), tracking(false), controllerID(-1), indexTrigger(0.0f), handTrigger(0.0f), button{false, false, false, false, false, false} {}

		Type type;
		bool hasHaptics;
		double hapticsSampleLength;
		uint32 hapticsMaxSamples;

		bool active;
		bool tracking;
		uint32 controllerID;

		Vector3 positionAim;
		Quaternion rotationAim;

		Vector3 positionGrip;
		Quaternion rotationGrip;

		float indexTrigger;
		float handTrigger;
		Vector2 thumbstick;
		Vector2 trackpad;

		Vector3 velocityLinear;
		Vector3 velocityAngular;

		bool button[Button::BUTTON_COUNT];
	};

	struct VRControllerHaptics
	{
		VRControllerHaptics() : sampleCount(0) {}
		void Push(float sample)
		{
			samples[sampleCount++] = sample;
		}

		float samples[256];
		uint8 sampleCount;
	};
}


#endif /* __RAYNE_VRTRACKINGSTATE_H_ */
