//
//  RNBHapticsTypes.cpp
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBHapticsTypes.h"

namespace RN
{
	RNDefineMeta(BHapticsDevice, Object)

	BHapticsDevice::BHapticsDevice() : address(nullptr), deviceName(nullptr), position(BHapticsDevicePosition::Default), isConnected(false), isPaired(false)
	{
		
	}

	BHapticsDevice::~BHapticsDevice()
	{
		SafeRelease(address);
		SafeRelease(deviceName);
	}
}
