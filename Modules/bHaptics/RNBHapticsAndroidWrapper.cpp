//
//  RNBHapticsAndroidWrapper.cpp
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBHapticsAndroidWrapper.h"

namespace RN
{
	bool BHapticsAndroidWrapper::_isBhapticsAvailable = false;
	bool BHapticsAndroidWrapper::_isBhapticsAvailableChecked = false;

#if RN_PLATFORM_ANDROID
    jmethodID PlayMethodId;
	jmethodID PlayDotMethodId;
	jmethodID PlayGloveMethodId;
	jmethodID PlayLoopMethodId;
	jmethodID InitializeMethodId;
	jmethodID InitializePermissionMethodId;

	jmethodID IsBhapticsAvailableMethodId;

	jmethodID IsPlayingByRequestIdMethodId;
	jmethodID IsPlayingByEventIdMethodId;
	jmethodID IsPlayingMethodId;

	jmethodID GetDevicesMethodId;
	jmethodID PingMethodId;
	jmethodID PingAllMethodId;
	jmethodID SwapPositionMethodId;


	jmethodID StopAllMethodId;
	jmethodID StopByRequestIdMethodId;
	jmethodID StopByEventIdMethodId;


/*	RN_JNI_METHOD void Java_com_slindev_grab_CODNativeActivity_onRefreshPairedInfo(JNIEnv* jenv, jobject thiz)
	{
		//BHapticsAndroidWrapper::_isBhapticsAvailableChecked = false;
	}

    RN_JNI_METHOD jint JNI_OnLoad(JavaVM *vm, void *)
    {
		return RN_JNI_VERSION_1_6;
    }*/
#endif

	void BHapticsAndroidWrapper::Initialize(const String *applicationID, const String *apiKey, const String *defaultConfig, bool requestPermission)
	{
	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);

			PlayMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_Play", "(Ljava/lang/String;FFFF)I");
			PlayDotMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_PlayDot", "(II[I)I");
			PlayGloveMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_PlayGlove", "(I[I[I[I)I");
			PlayLoopMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_PlayLoop", "(Ljava/lang/String;FFFFII)I");

			InitializeMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_Initialize", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
			InitializePermissionMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_InitializeWithPermission", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Z)V");
			IsBhapticsAvailableMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_IsBhapticsAvailable", "()Z");

			IsPlayingByEventIdMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_IsPlayingByEventId", "(Ljava/lang/String;)Z");
			IsPlayingByRequestIdMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_IsPlayingByRequestId", "(I)Z");
			IsPlayingMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_IsPlaying", "()Z");

			SwapPositionMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_SwapPosition", "(Ljava/lang/String;)V");
			PingMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_Ping", "(Ljava/lang/String;)V");
			PingAllMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_PingAll", "()V");

			GetDevicesMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_getDeviceList", "()Ljava/lang/String;");

			StopAllMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_StopAll", "()Z");
			StopByRequestIdMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_StopByRequestId", "(I)Z");
			StopByEventIdMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_StopByEventId", "(Ljava/lang/String;)Z");


			jstring appStrJava = env->NewStringUTF(applicationID->GetUTF8String());
			jstring keyStrJava = env->NewStringUTF(apiKey->GetUTF8String());
			jstring defaultConfigJava = env->NewStringUTF(defaultConfig->GetUTF8String());

			env->CallVoidMethod(app->activity->clazz, InitializePermissionMethodId, appStrJava, keyStrJava, defaultConfigJava, requestPermission);

			env->DeleteLocalRef(appStrJava);
			env->DeleteLocalRef(keyStrJava);
			env->DeleteLocalRef(defaultConfigJava);

			IsBhapticsAvailable();

			NotificationManager::GetSharedInstance()->AddSubscriber(kRNAndroidOnResume, [](Notification *notification){
				BHapticsAndroidWrapper::_isBhapticsAvailableChecked = false;
				BHapticsAndroidWrapper::_isBhapticsAvailable = false;
				BHapticsAndroidWrapper::IsBhapticsAvailable();
			}, RNCSTR("BHapticsAndroidWrapper"));
		}
	#endif
	}


	BHapticsDevicePosition BHapticsAndroidWrapper::StringToDevicePosition(const String *positionString)
	{
		if(positionString->IsEqual(RNCSTR("ForearmL")))
		{
			return BHapticsDevicePosition::ForearmL;
		}
		else if(positionString->IsEqual(RNCSTR("ForearmR")))
		{
			return BHapticsDevicePosition::ForearmR;
		}
		else if(positionString->IsEqual(RNCSTR("Vest")))
		{
			return BHapticsDevicePosition::Vest;
		}
		else if(positionString->IsEqual(RNCSTR("VestFront")))
		{
			return BHapticsDevicePosition::VestFront;
		}
		else if(positionString->IsEqual(RNCSTR("VestBack")))
		{
			return BHapticsDevicePosition::VestBack;
		}
		else if(positionString->IsEqual(RNCSTR("Head")))
		{
			return BHapticsDevicePosition::Head;
		}
		else if(positionString->IsEqual(RNCSTR("HandL")))
		{
			return BHapticsDevicePosition::HandL;
		}
		else if(positionString->IsEqual(RNCSTR("HandR")))
		{
			return BHapticsDevicePosition::HandR;
		}
		else if(positionString->IsEqual(RNCSTR("FootL")))
		{
			return BHapticsDevicePosition::FootL;
		}
		else if(positionString->IsEqual(RNCSTR("FootR")))
		{
			return BHapticsDevicePosition::FootR;
		}
		
		return BHapticsDevicePosition::Default;
	}

	const String *BHapticsAndroidWrapper::DevicePositionToString(BHapticsDevicePosition position)
	{
		if(position == BHapticsDevicePosition::ForearmL)
		{
			return RNCSTR("ForearmL");
		}
		else if(position == BHapticsDevicePosition::ForearmR)
		{
			return RNCSTR("ForearmR");
		}
		else if(position == BHapticsDevicePosition::Vest)
		{
			return RNCSTR("Vest");
		}
		else if(position == BHapticsDevicePosition::VestFront)
		{
			return RNCSTR("VestFront");
		}
		else if(position == BHapticsDevicePosition::VestBack)
		{
			return RNCSTR("VestBack");
		}
		else if(position == BHapticsDevicePosition::Head)
		{
			return RNCSTR("Head");
		}
		else if(position == BHapticsDevicePosition::HandL)
		{
			return RNCSTR("HandL");
		}
		else if(position == BHapticsDevicePosition::HandR)
		{
			return RNCSTR("HandR");
		}
		else if(position == BHapticsDevicePosition::FootL)
		{
			return RNCSTR("FootL");
		}
		else if(position == BHapticsDevicePosition::FootR)
		{
			return RNCSTR("FootR");
		}
		
		return nullptr;
	}



	bool BHapticsAndroidWrapper::IsBhapticsAvailable()
	{
#if RN_PLATFORM_ANDROID
		if(_isBhapticsAvailableChecked)
		{
			return _isBhapticsAvailable;
		}

		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			_isBhapticsAvailable = env->CallBooleanMethod(app->activity->clazz, IsBhapticsAvailableMethodId);
			_isBhapticsAvailableChecked = true;
		}

		return _isBhapticsAvailable;
#endif

		return false;
	}

	const Array *BHapticsAndroidWrapper::GetCurrentDevices()
    {
        Array *devices = nullptr;

        if(!_isBhapticsAvailable)
		{
			return devices;
		}

#if RN_PLATFORM_ANDROID
        android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
            jstring jstr = (jstring)env->CallObjectMethod(app->activity->clazz, GetDevicesMethodId);
			if(jstr == nullptr) return devices;

            jsize stringLength = env->GetStringUTFLength(jstr);
            if(stringLength == 0) return devices;

            const char* nativeDeviceListString = env->GetStringUTFChars(jstr, 0);
            const String *deviceListString = RN::String::WithBytes(nativeDeviceListString, stringLength, RN::Encoding::UTF8);
            env->ReleaseStringUTFChars(jstr, nativeDeviceListString);
			env->DeleteLocalRef(jstr);

            const Array *jsonDevices = JSONSerialization::ObjectFromString<RN::Array>(deviceListString);
            if(jsonDevices && jsonDevices->GetCount() > 0)
            {
                devices = new RN::Array(jsonDevices->GetCount());
                jsonDevices->Enumerate<Dictionary>([&](Dictionary *dict, size_t index, bool &stop){
                    BHapticsDevice *device = new BHapticsDevice();

					device->deviceName = SafeRetain(dict->GetObjectForKey<String>(RNCSTR("DeviceName")));
					device->address = SafeRetain(dict->GetObjectForKey<String>(RNCSTR("Address")));

                    device->position = StringToDevicePosition(dict->GetObjectForKey<String>(RNCSTR("Position")));

                    const Number *isConnectedNumber = dict->GetObjectForKey<Number>(RNCSTR("IsConnected"));
                    device->isConnected = isConnectedNumber->GetBoolValue();

                    const Number *isPairedNumber = dict->GetObjectForKey<Number>(RNCSTR("IsPaired"));
                    device->isPaired = isPairedNumber->GetBoolValue();

                    devices->AddObject(device);
                });
            }
        }
#endif

        return devices;
    }


	bool BHapticsAndroidWrapper::PlayHaptic(const String *eventName)
	{
		return Play(eventName, 1, 1, 0, 0);
	}

	bool BHapticsAndroidWrapper::Play(const String *eventName, float intensity, float duration, float angleX, float offsetY)
	{
#if RN_PLATFORM_ANDROID
		if(!_isBhapticsAvailable)
		{
			return false;
		}
	
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			jstring eventIdJava = env->NewStringUTF(eventName->GetUTF8String());
			int res = env->CallIntMethod(app->activity->clazz, PlayMethodId, eventIdJava, intensity, duration, angleX, offsetY);
			env->DeleteLocalRef(eventIdJava);
			return res;
		}
	#endif

		return false;
	}

/*	int BhapticsRequest::PlayDot(int position, float duration, TArray<int> motorValues)
	{
		if (!_isBhapticsAvailable)
		{
			return -1;
		}

		int durationInt = (int)(duration * 1000);
		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::PlayDot"));
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jintArray motorValuesJava = Env->NewIntArray(motorValues.Num());
			jint* indexes = new jint[motorValues.Num()];
			for (int i = 0; i < motorValues.Num(); ++i) {
				indexes[i] = FMath::Clamp(motorValues[i], 0, 100);
			}
			Env->SetIntArrayRegion(motorValuesJava, 0, motorValues.Num(), indexes);

			int res = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis, PlayDotMethodId, position, durationInt, motorValuesJava);
			return res;
		}
		return -1;

	#elif PLATFORM_WINDOWS
		int* motors = new int[motorValues.Num()];
		for (int i = 0; i < motorValues.Num(); ++i) {
			motors[i] = FMath::Clamp(motorValues[i], 0, 100);
		}
		return playDot(position, durationInt, motors, motorValues.Num());
	#endif
	}

	int BhapticsRequest::PlayWaveform(int position, TArray<int> motorIntensities, TArray<EBhapticsGlovePlayTime> playTimeValues, TArray<EBhapticsGloveShapeValue> shapeValues)
	{
		if (!_isBhapticsAvailable)
		{
			return -1;
		}

		if (motorIntensities.Num() != 6 || playTimeValues.Num() != 6 || shapeValues.Num() != 6) 
		{
			UE_LOG(BhapticsPlugin, Error, TEXT("BhapticsRequest::PlayGlove  - 'motorValues, playTimeValues, shapeValues' necessarily require 6 values each."));
			return -1;
		}


		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::PlayGlove"));
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jintArray motorIntensitiesJava = Env->NewIntArray(motorIntensities.Num());
			jintArray playTimeValuesJava = Env->NewIntArray(playTimeValues.Num());
			jintArray shapeValuesJava = Env->NewIntArray(shapeValues.Num());

			jint* motors = new jint[motorIntensities.Num()];
			for (int i = 0; i < motorIntensities.Num(); ++i) {
				motors[i] = FMath::Clamp(motorIntensities[i], 0, 100);
			}

			jint* playTimes = new jint[playTimeValues.Num()];
			jint* tempShapeValues = new jint[shapeValues.Num()];
			for (int i = 0; i < playTimeValues.Num(); ++i) {
				playTimes[i] = (int)playTimeValues[i];
			}

			for (int i = 0; i < shapeValues.Num(); ++i) {
				tempShapeValues[i] = (int)shapeValues[i];
			}


			Env->SetIntArrayRegion(motorIntensitiesJava, 0, motorIntensities.Num(), motors);
			Env->SetIntArrayRegion(playTimeValuesJava, 0, playTimeValues.Num(), playTimes);
			Env->SetIntArrayRegion(shapeValuesJava, 0, shapeValues.Num(), tempShapeValues);

			int res = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis, PlayGloveMethodId, position, motorIntensitiesJava, playTimeValuesJava, shapeValuesJava);
			return res;
		}
		return -1;

	#elif PLATFORM_WINDOWS
		int* motors = new int[motorIntensities.Num()];
		for (int i = 0; i < motorIntensities.Num(); ++i) {
			motors[i] = FMath::Clamp(motorIntensities[i], 0, 100);
		}
		int* playTimes = new int[playTimeValues.Num()];
		for (int i = 0; i < playTimeValues.Num(); ++i) {
			playTimes[i] = (int)playTimeValues[i];
		}
		int* tempShapeValues = new int[shapeValues.Num()];
		for (int i = 0; i < shapeValues.Num(); ++i) {
			tempShapeValues[i] = (int)shapeValues[i];
		}
		return playWaveform(position, motors, playTimes, tempShapeValues, 6);
	#endif
	}

	int BhapticsRequest::PlayLoop(FString eventId, float intensity, float duration, float angleX, float offsetY, int interval, int maxCount)
	{
		if (!_isBhapticsAvailable)
		{
			return -1;
		}

	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jstring eventIdJava = Env->NewStringUTF(TCHAR_TO_UTF8(*eventId));
			int res = FJavaWrapper::CallIntMethod(Env, FJavaWrapper::GameActivityThis, PlayLoopMethodId, eventIdJava, intensity, duration, angleX, offsetY, interval, maxCount);
			Env->DeleteLocalRef(eventIdJava);
			return res;
		}
		return -1;
	#elif PLATFORM_WINDOWS
		std::string eventNameStr(TCHAR_TO_UTF8(*eventId));
		return playLoop(eventNameStr.c_str(), intensity, duration, angleX, offsetY, interval, maxCount);
	#endif
	}*/

	bool BHapticsAndroidWrapper::IsPlaying()
	{
		if(!_isBhapticsAvailable)
		{
			return false;
		}


	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			bool res = env->CallBooleanMethod(app->activity->clazz, IsPlayingMethodId);
			return res;
		}
	#endif

		return false;
	}
/*
	bool BHapticsAndroidWrapper::IsPlayingByRequestId(int requestId)
	{
		if (!_isBhapticsAvailable)
		{
			return false;
		}

	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			bool res = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, IsPlayingByRequestIdMethodId, requestId);
			return res;
		}
		return false;
	#elif PLATFORM_WINDOWS
		return isPlayingByRequestId(requestId);
	#endif
	}*/

	bool BHapticsAndroidWrapper::IsPlayingByEventName(const String *eventName)
	{
		if (!_isBhapticsAvailable)
		{
			return false;
		}

	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			jstring eventIdJava = env->NewStringUTF(eventName->GetUTF8String());
			bool res = env->CallBooleanMethod(app->activity->clazz, IsPlayingByEventIdMethodId, eventIdJava);
			env->DeleteLocalRef(eventIdJava);
			return res;
		}
	#endif

		return false;
	}

	void BHapticsAndroidWrapper::Ping(const String *deviceAddress)
	{
		if (!_isBhapticsAvailable)
		{
			return;
		}

	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			jstring deviceIdJava = env->NewStringUTF(deviceAddress->GetUTF8String());
			env->CallVoidMethod(app->activity->clazz, PingMethodId, deviceIdJava);
			env->DeleteLocalRef(deviceIdJava);
		}
	#endif
	}

	void BHapticsAndroidWrapper::PingAll()
	{
		if(!_isBhapticsAvailable)
		{
			return;
		}

	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			env->CallVoidMethod(app->activity->clazz, PingAllMethodId);
		}
	#endif
	}

/*	void BHapticsAndroidWrapper::SwapPosition(FBhapticsDevice device)
	{
		if (!_isBhapticsAvailable)
		{
			return;
		}

		std::string addrStr(TCHAR_TO_UTF8(*device.Address));

	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jstring deviceIdJava = Env->NewStringUTF(TCHAR_TO_UTF8(*device.Address));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, SwapPositionMethodId, deviceIdJava);
			Env->DeleteLocalRef(deviceIdJava);
		}
	#elif PLATFORM_WINDOWS
		swapPosition(addrStr.c_str());
	#endif
	}*/

	bool BHapticsAndroidWrapper::StopByEventName(const String *eventName)
	{
		if(!_isBhapticsAvailable)
		{
			return false;
		}

	    //UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::StopByEventId"));
	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			jstring eventIdJava = env->NewStringUTF(eventName->GetUTF8String());
			bool res = env->CallBooleanMethod(app->activity->clazz, StopByEventIdMethodId, eventIdJava);
			env->DeleteLocalRef(eventIdJava);
			return res;
		}
	#endif

		return false;
	}

/*	bool BHapticsAndroidWrapper::StopByRequestId(int requestId)
	{
		if (!_isBhapticsAvailable)
		{
			return false;
		}

		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::StopByRequestId"));
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			bool res = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, StopByRequestIdMethodId, requestId);
			return res;
		}


		return false;
	#elif PLATFORM_WINDOWS
		return stop(requestId);
	#endif
	}*/

	bool BHapticsAndroidWrapper::Stop()
	{
		if(!_isBhapticsAvailable)
		{
			return false;
		}

		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::Stop"));
	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
        JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
        if(env)
        {
			bool res = env->CallBooleanMethod(app->activity->clazz, StopAllMethodId);
			return res;
		}
	#endif

		return false;
	}

	void BHapticsAndroidWrapper::Destroy()
	{
	#if RN_PLATFORM_ANDROID
		//Nothing to do
	#endif
	}



/*	FBhapticsRotationOption BHapticsAndroidWrapper::ProjectToVest(FVector Location, UPrimitiveComponent* HitComponent, float HalfHeight) 
	{
		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::ProjectToVest"));

		if (HitComponent == nullptr)
		{
			return FBhapticsRotationOption(0, 0);
		}

		FRotator InverseRotation = HitComponent->GetComponentRotation().GetInverse();
		FVector HitPoint = InverseRotation.RotateVector(Location - HitComponent->GetComponentLocation());
		FVector UpVector = FVector::UpVector;//InverseRotation.RotateVector(HitComponent->GetUpVector());
		FVector ForwardVector = FVector::ForwardVector;//InverseRotation.RotateVector(HitComponent->GetForwardVector());
		FVector Scale = HitComponent->GetComponentScale();
		float DotProduct, Angle, Y_Offset, A, B;
		FVector Result;

		UpVector.Normalize();
		ForwardVector.Normalize();

		HitPoint.X = HitPoint.X / Scale.X;
		HitPoint.Y = HitPoint.Y / Scale.Y;
		HitPoint.Z = HitPoint.Z / Scale.Z;

		DotProduct = FVector::DotProduct(HitPoint, UpVector);

		Result = HitPoint - (DotProduct * UpVector);
		Result.Normalize();

		A = Result.X * ForwardVector.Y - ForwardVector.X * Result.Y;
		B = ForwardVector.X * Result.X + Result.Y * ForwardVector.Y;

		Angle = FMath::RadiansToDegrees(FMath::Atan2(A, B));

		if (HalfHeight < 0.01)
		{
			Y_Offset = 0;
		}
		else
		{
			Y_Offset = FMath::Clamp(DotProduct / (HalfHeight * 2), -0.5f, 0.5f);
		}

		return FBhapticsRotationOption(Angle, Y_Offset);
	}

	FBhapticsRotationOption BHapticsAndroidWrapper::ProjectToVestLocation(FVector ContactLocation, FVector PlayerLocation, FRotator PlayerRotation)
	{
		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::ProjectToVestLocation"));

		FRotator InverseRotation = PlayerRotation.GetInverse();
		FVector HitPoint = InverseRotation.RotateVector(ContactLocation - PlayerLocation);
		FVector UpVector = FVector::UpVector;//InverseRotation.RotateVector(HitComponent->GetUpVector());
		FVector ForwardVector = FVector::ForwardVector;//InverseRotation.RotateVector(HitComponent->GetForwardVector());
		float DotProduct, Angle, A, B;
		FVector Result;

		UpVector.Normalize();
		ForwardVector.Normalize();

		DotProduct = FVector::DotProduct(HitPoint, UpVector);

		Result = HitPoint - (DotProduct * UpVector);
		Result.Normalize();

		A = Result.X * ForwardVector.Y - ForwardVector.X * Result.Y;
		B = ForwardVector.X * Result.X + Result.Y * ForwardVector.Y;

		Angle = FMath::RadiansToDegrees(FMath::Atan2(A, B));

		return FBhapticsRotationOption(Angle, 0);
	}

	FBhapticsRotationOption BHapticsAndroidWrapper::CustomProjectToVest(FVector Location, UPrimitiveComponent* HitComponent, float HalfHeight, FVector UpVector, FVector ForwardVector)
	{
		//UE_LOG(BhapticsPlugin, Log, TEXT("BhapticsRequest::CustomProjectToVest"));

		if (HitComponent == nullptr)
		{
			return FBhapticsRotationOption(0, 0);
		}

		FRotator InverseRotation = HitComponent->GetComponentRotation().GetInverse();
		FVector HitPoint = InverseRotation.RotateVector(Location - HitComponent->GetComponentLocation());
		FVector Scale = HitComponent->GetComponentScale();
		float DotProduct, Angle, Y_Offset, A, B;
		FVector Result;

		if (UpVector == FVector::ZeroVector)
		{
			UpVector = InverseRotation.RotateVector(HitComponent->GetUpVector());
		}
		else
		{
			UpVector = InverseRotation.RotateVector(UpVector);
		}

		if (ForwardVector == FVector::ZeroVector)
		{
			ForwardVector = InverseRotation.RotateVector(HitComponent->GetForwardVector());
		}
		else
		{
			ForwardVector = InverseRotation.RotateVector(ForwardVector);
		}


		UpVector.Normalize();
		ForwardVector.Normalize();

		HitPoint.X = HitPoint.X / Scale.X;
		HitPoint.Y = HitPoint.Y / Scale.Y;
		HitPoint.Z = HitPoint.Z / Scale.Z;

		DotProduct = FVector::DotProduct(HitPoint, UpVector);

		Result = HitPoint - (DotProduct * UpVector);
		Result.Normalize();

		A = Result.X * ForwardVector.Y - ForwardVector.X * Result.Y + Result.Y * ForwardVector.Z - ForwardVector.Y * Result.Z + Result.Z * ForwardVector.X - ForwardVector.Z * Result.X;
		B = ForwardVector.X * Result.X + Result.Y * ForwardVector.Y + Result.Z * ForwardVector.Z;

		Angle = FMath::RadiansToDegrees(FMath::Atan2(A, B));

		Y_Offset = FMath::Clamp(DotProduct / (HalfHeight * 2), -0.5f, 0.5f);

		return FBhapticsRotationOption(Angle, Y_Offset);
	}*/
}
