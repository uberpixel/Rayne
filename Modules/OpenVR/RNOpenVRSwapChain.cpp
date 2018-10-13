//
//  RNOpenVRSwapChain.cpp
//  Rayne-OpenVR
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenVRSwapChain.h"

namespace RN
{
	const uint32 OpenVRSwapChain::kEyePadding = 16; //Use a padding of 16 pixels (oculus docs recommend 8, but it isn't enough)

	OpenVRSwapChain::OpenVRSwapChain(vr::IVRSystem *system) : _vrSystem(system)
	{

	}

	OpenVRSwapChain::~OpenVRSwapChain()
	{
		SafeRelease(_targetTexture);
	}

	void OpenVRSwapChain::UpdatePredictedPose()
	{
		vr::VRCompositor()->WaitGetPoses(_frameDevicePose, vr::k_unMaxTrackedDeviceCount, _predictedDevicePose, vr::k_unMaxTrackedDeviceCount);
	}
}
