//
//  RNBHapticsAndroidWrapper.cpp
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BHAPTICS_ANDROID_WRAPPER_H_
#define __RAYNE_BHAPTICS_ANDROID_WRAPPER_H_

#include "RNBHaptics.h"
#include "RNBHapticsTypes.h"

namespace RN
{
	class BHapticsAndroidWrapper
	{
	public:
		static void Initialize();

		static void RegisterProject(const String *key, const String *fileStr);
		static void RegisterProjectReflected(const String *key, const String *fileStr);

		static void SubmitRegistered(const String *key, const String *altKey, float intensity, float duration, float xOffsetAngle, float yOffset);
		static void SubmitDot(const String *key, BHapticsDevicePosition devicePosition, const std::vector<BHapticsDotPoint> &points, int durationMillis);
		//static void SubmitPath(FString Key, FString Pos, TArray<FPathPoint> Points, int DurationMillis);


		static bool IsFeedbackRegistered(String *key);
		static bool IsFeedbackPlaying(String *key);
		static bool IsAnyFeedbackPlaying();
		//static TArray<uint8> GetPositionStatus(FString pos);

		static void TurnOffFeedback(const String *key);
		static void TurnOffAllFeedback();
		
		
		//Device management stuff that may not be important, but could be useful
		
		static const Array *GetCurrentDevices();
		
/*		static bool IsDeviceConnected(BHapticsDevicePosition devicePosition);

		//Set the Position of Device
		static void ChangeDevicePosition(const String *deviceAddress, BHapticsDevicePosition devicePosition);
		
		//Toggle the Position of Device
		static void ToggleDevicePosition(const String *deviceAddress);*/

		//Ping Haptic Device
		static void PingDevice(const String *deviceAddress);

		//Ping all Haptic Devices
		static void PingAllDevices();
		
		//Device management stuff below, probably not needed on Quest!?
		
		//static bool IsLegacyMode(); //Android display name is "IsStandAloneMode"!? Likely not relevant on quest anymore...
		
		//Scan for bHaptics devices
/*		static void StartScanning();

		//Stop scanning for devices
		static void StopScanning();

		//Check if its currently scanning
		static bool IsScanning();*/

		/*//Pair with Haptic Device
		static void PairDevice(FString DeviceAddress);

		//Pair with Haptic Device and set Position
		static void PairDeviceFromPosition(FString DeviceAddress, FString DevicePosition);

		//Unpair Haptic Device
		static void UnpairDevice(FString DeviceAddress);
		
		//Unpair all Haptic Devices
		static void UnpairAllDevices();*/
		   
	private:
		static BHapticsDevicePosition StringToDevicePosition(const String *positionString);
		static const String *DevicePositionToString(BHapticsDevicePosition position);
	};
}

#endif //__RAYNE_BHAPTICS_ANDROID_WRAPPER_H_
