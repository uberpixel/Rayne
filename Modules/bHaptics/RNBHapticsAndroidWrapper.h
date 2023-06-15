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
		static void Initialize(const String *applicationID, const String *apiKey, const String *defaultConfig, bool requestPermission);

		static bool IsBhapticsAvailable();
		static const Array *GetCurrentDevices();

		static bool PlayHaptic(const String *eventName);
		static bool Play(const String *eventName, float intensity, float duration, float angleX, float offsetY);
		//int PlayDot(int position, float duration, TArray<int> motorValues);
		//int PlayWaveform(int position, TArray<int> motorIntensities, TArray<EBhapticsGlovePlayTime> playTimeValues, TArray<EBhapticsGloveShapeValue> shapeValues);
		//int PlayLoop(FString eventId, float intensity, float duration, float angleX, float offsetY, int interval, int maxCount);

		static bool IsPlaying();
		//bool IsPlayingByRequestId(int requestId);
		static bool IsPlayingByEventName(const String *eventName);

		static void Ping(const String *deviceAddress);
		static void PingAll();

		//void SwapPosition(FBhapticsDevice device);
		static bool StopByEventName(const String *eventName);
		//bool StopByRequestId(int requestId);
		static bool Stop();
		static void Destroy();
		//FBhapticsRotationOption ProjectToVest(FVector Location, UPrimitiveComponent* HitComponent, float HalfHeight);
		//FBhapticsRotationOption ProjectToVestLocation(FVector ContactLocation, FVector PlayerLocation, FRotator PlayerRotation);
		//FBhapticsRotationOption CustomProjectToVest(FVector Location, UPrimitiveComponent* HitComponent, float HalfHeight, FVector UpVector, FVector ForwardVector);

		static bool _isBhapticsAvailableChecked;
		   
	private:
		static BHapticsDevicePosition StringToDevicePosition(const String *positionString);
		static const String *DevicePositionToString(BHapticsDevicePosition position);

		static bool _isBhapticsAvailable;
	};
}

#endif //__RAYNE_BHAPTICS_ANDROID_WRAPPER_H_
