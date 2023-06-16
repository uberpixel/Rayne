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

	BHapticsDevice::BHapticsDevice() : address(nullptr), deviceName(nullptr), position(BHapticsDevicePosition::Vest), isConnected(false), isPaired(false)
	{
		
	}

	BHapticsDevice::~BHapticsDevice()
	{
		SafeRelease(address);
		SafeRelease(deviceName);
	}

	BHapticsDevicePosition BHapticsDevice::StringToDevicePosition(const String *positionString)
	{
		if (positionString->IsEqual(RNCSTR("ForearmL")))
		{
			return BHapticsDevicePosition::ForearmL;
		}
		else if (positionString->IsEqual(RNCSTR("ForearmR")))
		{
			return BHapticsDevicePosition::ForearmR;
		}
		else if (positionString->IsEqual(RNCSTR("Vest")))
		{
			return BHapticsDevicePosition::Vest;
		}
		else if (positionString->IsEqual(RNCSTR("Head")))
		{
			return BHapticsDevicePosition::Head;
		}
		else if (positionString->IsEqual(RNCSTR("HandL")))
		{
			return BHapticsDevicePosition::HandL;
		}
		else if (positionString->IsEqual(RNCSTR("HandR")))
		{
			return BHapticsDevicePosition::HandR;
		}
		else if (positionString->IsEqual(RNCSTR("FootL")))
		{
			return BHapticsDevicePosition::FootL;
		}
		else if (positionString->IsEqual(RNCSTR("FootR")))
		{
			return BHapticsDevicePosition::FootR;
		}

		return BHapticsDevicePosition::Vest;
	}

	const String *BHapticsDevice::DevicePositionToString(BHapticsDevicePosition position)
	{
		if (position == BHapticsDevicePosition::ForearmL)
		{
			return RNCSTR("ForearmL");
		}
		else if (position == BHapticsDevicePosition::ForearmR)
		{
			return RNCSTR("ForearmR");
		}
		else if (position == BHapticsDevicePosition::Vest)
		{
			return RNCSTR("Vest");
		}
		else if (position == BHapticsDevicePosition::Head)
		{
			return RNCSTR("Head");
		}
		else if (position == BHapticsDevicePosition::HandL)
		{
			return RNCSTR("HandL");
		}
		else if (position == BHapticsDevicePosition::HandR)
		{
			return RNCSTR("HandR");
		}
		else if (position == BHapticsDevicePosition::FootL)
		{
			return RNCSTR("FootL");
		}
		else if (position == BHapticsDevicePosition::FootR)
		{
			return RNCSTR("FootR");
		}

		return nullptr;
	}
}
