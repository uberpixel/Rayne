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
	#if RN_PLATFORM_ANDROID
	jmethodID InitializeMethod;
	jmethodID GetCurrentDeviceMethod;
	jmethodID StartScanMethod;
	jmethodID StopScanMethod;
	jmethodID IsScanningMethod;
	jmethodID isRegisterMethodId;
	jmethodID GetPositionStatusMethodId;
	#endif

	void BHapticsAndroidWrapper::Initialize()
	{
	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);

			InitializeMethod = env->GetMethodID(activityClass, "AndroidThunkJava_Initialize", "(Ljava/lang/String;)V");
			GetCurrentDeviceMethod = env->GetMethodID(activityClass, "AndroidThunkJava_getDeviceList", "()Ljava/lang/String;");
			StartScanMethod = env->GetMethodID(activityClass, "AndroidThunkJava_Scan", "()V");
			StopScanMethod = env->GetMethodID(activityClass, "AndroidThunkJava_StopScan", "()V");
			IsScanningMethod = env->GetMethodID(activityClass, "AndroidThunkJava_IsScanning", "()Z");
			GetPositionStatusMethodId =
			env->GetMethodID(activityClass, "AndroidThunkJava_GetPositionStatus", "(Ljava/lang/String;)[B");

			//This doesn't currently do anything, except printing the app name, but would be nice to figure out how to cleanup the wrapper in the activity for this to do the init
            jstring applicationName = env->NewStringUTF("GRAB");
			env->CallVoidMethod(app->activity->clazz, InitializeMethod, applicationName);
            env->DeleteLocalRef(applicationName);
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


	const Array *BHapticsAndroidWrapper::GetCurrentDevices()
	{
		Array *devices = nullptr;

	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jstring jstr = (jstring)env->CallObjectMethod(app->activity->clazz, GetCurrentDeviceMethod);

			const char* nativeDeviceListString = env->GetStringUTFChars(jstr, 0);
			const String *deviceListString = RNSTR(nativeDeviceListString);
			env->ReleaseStringUTFChars(jstr, nativeDeviceListString);

			RNDebug("bHaptics devices: " << deviceListString);

			const Array *jsonDevices = JSONSerialization::ObjectFromString<RN::Array>(deviceListString);
			if(jsonDevices && jsonDevices->GetCount() > 0)
			{
				devices = new RN::Array(jsonDevices->GetCount());
				jsonDevices->Enumerate<Dictionary>([&](Dictionary *dict, size_t index, bool &stop){
					BHapticsDevice *device = new BHapticsDevice();

					//TODO: Also set the properties below
					//DeviceName
					//Address

					device->Position = StringToDevicePosition(dict->GetObjectForKey<String>(RNCSTR("Position")));

					const Number *isConnectedNumber = dict->GetObjectForKey<Number>(RNCSTR("IsConnected"));
					device->IsConnected = isConnectedNumber->GetBoolValue();

					const Number *isPairedNumber = dict->GetObjectForKey<Number>(RNCSTR("IsPaired"));
					device->IsPaired = isPairedNumber->GetBoolValue();

					devices->AddObject(device);
				});
			}
		}
    #endif

		return devices;
	}

	void BHapticsAndroidWrapper::PingDevice(const String *deviceAddress)
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID pingMethod = env->GetMethodID(activityClass, "AndroidThunkJava_Ping", "(Ljava/lang/String;)V");
			
			jstring deviceStrJava = env->NewStringUTF(deviceAddress->GetUTF8String());
			env->CallVoidMethod(app->activity->clazz, pingMethod, deviceStrJava);
			env->DeleteLocalRef(deviceStrJava);
		}
#endif
	}

	void BHapticsAndroidWrapper::PingAllDevices()
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID PingAllMethod = env->GetMethodID(activityClass, "AndroidThunkJava_PingAll", "()V");
			env->CallVoidMethod(app->activity->clazz, PingAllMethod);
		}
#endif
	}

	/*
	void UAndroidHapticLibrary::ChangeDevicePosition(FString DeviceAddress, FString Position)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID ChangePositionMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_ChangePosition", "(Ljava/lang/String;Ljava/lang/String;)V", false);
			jstring DeviceAddressJava = Env->NewStringUTF(TCHAR_TO_UTF8(*DeviceAddress));
			jstring PositionJava = Env->NewStringUTF(TCHAR_TO_UTF8(*Position));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ChangePositionMethod, DeviceAddressJava, PositionJava);
			Env->DeleteLocalRef(DeviceAddressJava);
			Env->DeleteLocalRef(PositionJava);
		}
	#endif
	}

	void UAndroidHapticLibrary::ToggleDevicePosition(FString DeviceAddress)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID TogglePositionMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_TogglePosition", "(Ljava/lang/String;)V", false);
			jstring DeviceAddressJava = Env->NewStringUTF(TCHAR_TO_UTF8(*DeviceAddress));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, TogglePositionMethod,DeviceAddressJava);
			Env->DeleteLocalRef(DeviceAddressJava);
		}
	#endif
	}
*/

	void BHapticsAndroidWrapper::SubmitDot(const String *key, BHapticsDevicePosition devicePosition, const std::vector<BHapticsDotPoint> &points, int durationMillis)
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID submitDotMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_SubmitDot", "(Ljava/lang/String;Ljava/lang/String;[I[II)V");

			jstring keyStrJava = env->NewStringUTF(key->GetUTF8String());
			jstring posJava = env->NewStringUTF(DevicePositionToString(devicePosition)->GetUTF8String());

			jintArray indexesJava = env->NewIntArray(points.size());
			jintArray intensitiesJava = env->NewIntArray(points.size());

			jint *indexes = new jint[points.size()];
			jint *intensities = new jint[points.size()];
			for(int i = 0; i < points.size(); ++i)
			{
				indexes[i] = points[i].Index;
				intensities[i] = points[i].Intensity;
			}
			env->SetIntArrayRegion(indexesJava, 0, points.size(), indexes);
			env->SetIntArrayRegion(intensitiesJava, 0, points.size(), intensities);

			env->CallVoidMethod(app->activity->clazz, submitDotMethodId, keyStrJava, posJava, indexesJava, intensitiesJava, durationMillis);

			env->DeleteLocalRef(keyStrJava);
			env->DeleteLocalRef(posJava);

			delete[] indexes;
			delete[] intensities;
		}
#endif
	}
/*
	void UAndroidHapticLibrary::SubmitPath(FString Key, FString Pos, TArray<FPathPoint> Points, int DurationMillis)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID submitDotMethodId =
				FJavaWrapper::FindMethod(
					Env, FJavaWrapper::GameActivityClassID,
					"AndroidThunkJava_SubmitPath", "(Ljava/lang/String;Ljava/lang/String;[F[F[II)V", false);
			jstring keyStrJava = Env->NewStringUTF(TCHAR_TO_UTF8(*Key));
			jstring posJava = Env->NewStringUTF(TCHAR_TO_UTF8(*Pos));

			jfloatArray xJava = Env->NewFloatArray(Points.Num());
			jfloatArray yJava = Env->NewFloatArray(Points.Num());
			jintArray intensitiesJava = Env->NewIntArray(Points.Num());
			jfloat* x = new jfloat[Points.Num()];
			jfloat* y = new jfloat[Points.Num()];
			jint* intensities = new jint[Points.Num()];
			for (int i = 0; i < Points.Num(); ++i) {
				x[i] = Points[i].X;
				y[i] = Points[i].Y;
				intensities[i] = Points[i].Intensity;
			}

			Env->SetFloatArrayRegion(xJava, 0, Points.Num(), x);
			Env->SetFloatArrayRegion(yJava, 0, Points.Num(), y);
			Env->SetIntArrayRegion(intensitiesJava, 0, Points.Num(), intensities);


			FJavaWrapper::CallVoidMethod(
				Env, FJavaWrapper::GameActivityThis, submitDotMethodId,
				keyStrJava, posJava, xJava, yJava, intensitiesJava, DurationMillis);
			Env->DeleteLocalRef(keyStrJava);
			Env->DeleteLocalRef(posJava);

			delete[] x;
			delete[] y;
			delete[] intensities;
		}
	#endif
	}*/

	void BHapticsAndroidWrapper::TurnOffFeedback(const String *key)
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID turnOffMethod = env->GetMethodID(activityClass, "AndroidThunkJava_TurnOff", "(Ljava/lang/String;)V");
			
			jstring keyStrJava = env->NewStringUTF(key->GetUTF8String());
			env->CallVoidMethod(app->activity->clazz, turnOffMethod, keyStrJava);
			env->DeleteLocalRef(keyStrJava);
		}
#endif
	}

	void BHapticsAndroidWrapper::TurnOffAllFeedback()
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID turnOffAll = env->GetMethodID(activityClass, "AndroidThunkJava_TurnOffAll", "()V");
			env->CallVoidMethod(app->activity->clazz, turnOffAll);
		}
#endif
	}

/*	bool BHapticsAndroidWrapper::IsDeviceConnected(BHapticsDevicePosition devicePosition)
	{
		auto Devices = UAndroidHapticLibrary::GetCurrentDevices();
		for (int i = 0; i < Devices.Num(); i++) {
			FDevice d = Devices[i];
			if (d.IsConnected
				&& BhapticsUtils::PositionEnumToString(Position) == d.Position) {
				return true;
			}
		}
		return false;
	}

	bool UAndroidHapticLibrary::IsFeedbackRegistered(FString key)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID isRegisterMethodId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsRegistered", "(Ljava/lang/String;)Z", false);
			jstring keyStrJava = Env->NewStringUTF(TCHAR_TO_UTF8(*key));
			bool res = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, isRegisterMethodId, keyStrJava);
			Env->DeleteLocalRef(keyStrJava);

			return res;
		}
	#endif

		return false;
	}

	bool UAndroidHapticLibrary::IsFeedbackPlaying(FString key)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID isRegisterMethodId =
				FJavaWrapper::FindMethod(
					Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsPlaying", "(Ljava/lang/String;)Z", false);
			jstring keyStrJava = Env->NewStringUTF(TCHAR_TO_UTF8(*key));
			bool res = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, isRegisterMethodId, keyStrJava);
			Env->DeleteLocalRef(keyStrJava);

			return res;
		}
	#endif

		return false;
	}

	bool UAndroidHapticLibrary::IsAnyFeedbackPlaying()
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID isRegisterMethodId =
				FJavaWrapper::FindMethod(
					Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_IsAnythingPlaying", "()Z", false);
			return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, isRegisterMethodId);
		}
	#endif
		return false;
	}

	TArray<uint8> UAndroidHapticLibrary::GetPositionStatus(FString pos)
	{
		TArray<uint8> IntArray;
		IntArray.Init(0, 20);
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			jstring posJava = Env->NewStringUTF(TCHAR_TO_UTF8(*pos));
		
			jbyteArray arrayJava = (jbyteArray) FJavaWrapper::CallObjectMethod(
				Env, FJavaWrapper::GameActivityThis, GetPositionStatusMethodId, posJava);

			Env->DeleteLocalRef(posJava);

			jbyte* byteArr = Env->GetByteArrayElements(arrayJava, 0);
			jsize length = Env->GetArrayLength(arrayJava);
			for (int posIndex = 0; posIndex < length; posIndex++)
			{
				IntArray[posIndex] = byteArr[posIndex];
			}

		}
	#endif

		return IntArray;
	}*/

	void BHapticsAndroidWrapper::RegisterProject(const String *key, const String *fileStr)
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			RNDebug(fileStr->GetLength());

			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID registerMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_Register", "(Ljava/lang/String;Ljava/lang/String;)V");
			
			jstring keyStrJava = env->NewStringUTF(key->GetUTF8String());
			jstring fileStrJava = env->NewStringUTF(fileStr->GetUTF8String());
			env->CallVoidMethod(app->activity->clazz, registerMethodId, keyStrJava, fileStrJava);
			env->DeleteLocalRef(keyStrJava);
			env->DeleteLocalRef(fileStrJava);
		}
#endif
	}

/*
	void UAndroidHapticLibrary::RegisterProjectReflected(FString key, FString fileStr)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID registerMethodId =
				FJavaWrapper::FindMethod(
					Env, FJavaWrapper::GameActivityClassID,
					"AndroidThunkJava_RegisterReflected",
					"(Ljava/lang/String;Ljava/lang/String;)V", false);
			jstring keyStrJava = Env->NewStringUTF(TCHAR_TO_UTF8(*key));
			jstring fileStrJava = Env->NewStringUTF(TCHAR_TO_UTF8(*fileStr));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, registerMethodId, keyStrJava, fileStrJava);
			Env->DeleteLocalRef(keyStrJava);
			Env->DeleteLocalRef(fileStrJava);
		}
	#endif
	}*/

	void BHapticsAndroidWrapper::SubmitRegistered(const String *key, const String *altKey, float intensity, float duration, float xOffsetAngle, float yOffset)
	{
#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			jclass activityClass = env->GetObjectClass(app->activity->clazz);
			static jmethodID submitMethodId = env->GetMethodID(activityClass, "AndroidThunkJava_SubmitRegistered", "(Ljava/lang/String;Ljava/lang/String;FFFF)V");
			
			jstring keyJava = env->NewStringUTF(key->GetUTF8String());
			jstring altKeyJava = env->NewStringUTF(altKey? altKey->GetUTF8String() : "");
			env->CallVoidMethod(app->activity->clazz, submitMethodId, keyJava, altKeyJava, intensity, duration, xOffsetAngle, yOffset);
			env->DeleteLocalRef(keyJava);
			env->DeleteLocalRef(altKeyJava);
		}
#endif
	}


/*
	bool UAndroidHapticLibrary::IsLegacyMode()
	{
	#if RN_PLATFORM_ANDROID
		if(JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID isLegacyModeId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Is_legacy", "()Z", false);
			bool res = FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, isLegacyModeId);

			return res;
		}
	#endif

		return false;
	}
 
	void BHapticsAndroidWrapper::StartScanning()
	{
	#if RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();
		if(env)
		{
			env->CallVoidMethod(app->activity->clazz, StartScanMethod);
		}
	#endif
	}

	void BHapticsAndroidWrapper::StopScanning()
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, StopScanMethod);
		}
	#endif
	}

	bool BHapticsAndroidWrapper::IsScanning()
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			return FJavaWrapper::CallBooleanMethod(Env, FJavaWrapper::GameActivityThis, IsScanningMethod);
		}
	#endif
		return false;
	}*/

	/*
	void UAndroidHapticLibrary::PairDevice(FString DeviceAddress)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID PairMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Pair", "(Ljava/lang/String;)V", false);
			jstring DeviceAddressJava = Env->NewStringUTF(TCHAR_TO_UTF8(*DeviceAddress));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, PairMethod, DeviceAddressJava);
			Env->DeleteLocalRef(DeviceAddressJava);
		}
	#endif
	}

	void UAndroidHapticLibrary::PairDeviceFromPosition(FString DeviceAddress, FString DevicePosition)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID PairMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_PairFromPosition", "(Ljava/lang/String;Ljava/lang/String;)V", false);
			jstring DeviceAddressJava = Env->NewStringUTF(TCHAR_TO_UTF8(*DeviceAddress));
			jstring DevicePositionJava = Env->NewStringUTF(TCHAR_TO_UTF8(*DevicePosition));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, PairMethod, DeviceAddressJava, DevicePositionJava);
			Env->DeleteLocalRef(DeviceAddressJava);
			Env->DeleteLocalRef(DevicePositionJava);
		}
	#endif
	}

	void UAndroidHapticLibrary::UnpairDevice(FString DeviceAddress)
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID UnpairMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_Unpair", "(Ljava/lang/String;)V", false);
			jstring DeviceAddressJava = Env->NewStringUTF(TCHAR_TO_UTF8(*DeviceAddress));
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, UnpairMethod,DeviceAddressJava);
			Env->DeleteLocalRef(DeviceAddressJava);
		}
	#endif
	}

	void UAndroidHapticLibrary::AndroidThunkCpp_UnpairAll()
	{
	#if RN_PLATFORM_ANDROID
		if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
		{
			static jmethodID UnpairAllMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_UnpairAll", "()V", false);
			FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, UnpairAllMethod);
		}
	#endif
	}*/
}
