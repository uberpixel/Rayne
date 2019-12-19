//
//  RNOpenALWorld.cpp
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenALWorld.h"

#if RN_PLATFORM_MAC_OS
#include <AVFoundation/AVFoundation.h>
#endif

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

namespace RN
{
	RNDefineMeta(OpenALWorld, SceneAttachment)
		
	OpenALWorld::OpenALWorld(String *outputDeviceName) :
		_audioListener(nullptr), _outputDevice(nullptr), _inputDevice(nullptr), _inputBuffer(nullptr), _inputBufferTemp(nullptr)
	{
		if(outputDeviceName)
			_outputDevice = alcOpenDevice(outputDeviceName->GetUTF8String());
		else
		    _outputDevice = alcOpenDevice(nullptr);
		if(!_outputDevice)
		{
			RNDebug("rayne-openal: Could not open output audio device.");
			return;
		}

		//Enable HRTF
		int attributes[3] = {ALC_HRTF_SOFT, ALC_TRUE, 0};
			
		_context = alcCreateContext(_outputDevice, attributes);
		alcMakeContextCurrent(_context);
		if(!_context)
		{
			RNDebug("rayne-openal: Could not create audio context.");
			return;
		}

		int hrtf_state = 0;
		alcGetIntegerv(_outputDevice, ALC_HRTF_SOFT, 1, &hrtf_state);
		if(!hrtf_state)
			RNDebug("HRTF not enabled!\n");
		else
		{
			const ALchar *name = alcGetString(_outputDevice, ALC_HRTF_SPECIFIER_SOFT);
			RNDebug("HRTF enabled, using " << name);
		}
	}
		
	OpenALWorld::~OpenALWorld()
	{
		if(_inputDevice)
		{
			alcCaptureStop(_inputDevice);
			alcCaptureCloseDevice(_inputDevice);
		}
		
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(_context);
		alcCloseDevice(_outputDevice);
		
		if(_inputBufferTemp)
		{
			delete[] _inputBufferTemp;
		}
	}

	void OpenALWorld::RequestMicrophonePermission()
	{
		MicrophonePermissionState permissionState = GetMicrophonePermissionState();
		if(permissionState == MicrophonePermissionStateNotDetermined)
		{
#if RN_PLATFORM_MAC_OS
			if (@available(macOS 10.14, *)) {
				[AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
					/* if(granted)
					 {
						_inputDevice = alcCaptureOpenDevice(inputDeviceName?inputDeviceName->GetUTF8String():nullptr, 48000, AL_FORMAT_MONO16, 480);
					 }*/
				 }];
			}
#elif RN_PLATFORM_ANDROID
			android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
			//JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();

			JNIEnv* env = nullptr;
			bool isNewEnv = false;

			switch(app->activity->vm->GetEnv((void**)&env, JNI_VERSION_1_8))
			{
				case JNI_OK:
					break;

				case JNI_EDETACHED:
				{
					jint attachresult = app->activity->vm->AttachCurrentThread(&env, nullptr);
					if(attachresult == JNI_ERR)
					{
						RNDebug("error attaching java env to threat");
						return;
					}

					isNewEnv = true;
					break;
				}

				case JNI_EVERSION:
					RNDebug("wrong jni version (should be 1.8)");
					return;
			}

			jclass activityClass = env->FindClass("android/app/NativeActivity");
			jmethodID getClassLoaderMethod = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
			jobject classLoaderObject = env->CallObjectMethod(app->activity->clazz, getClassLoaderMethod);
			jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
			jmethodID loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

			jstring activityCompatClassName = env->NewStringUTF("androidx.core.app.ActivityCompat");
			jclass activityCompatClass = reinterpret_cast<jclass>(env->CallObjectMethod(classLoaderObject, loadClassMethod, activityCompatClassName));
			env->DeleteLocalRef(activityCompatClassName);

			jobjectArray permissions = (jobjectArray)env->NewObjectArray(1, env->FindClass("java/lang/String"), env->NewStringUTF(""));
			env->SetObjectArrayElement(permissions, 0, env->NewStringUTF("android.permission.RECORD_AUDIO"));

			jint requestCode = 1;
			jmethodID requestPermissionsMethod = env->GetStaticMethodID(activityCompatClass, "requestPermissions", "(Landroid/app/Activity;[Ljava/lang/String;I)V");
			env->CallStaticVoidMethod(activityCompatClass, requestPermissionsMethod, app->activity->clazz, permissions, requestCode);
			
			env->DeleteLocalRef(permissions);

			if(isNewEnv)
			{
				app->activity->vm->DetachCurrentThread();
			}
#endif
		}
	}

	OpenALWorld::MicrophonePermissionState OpenALWorld::GetMicrophonePermissionState()
	{
#if RN_PLATFORM_MAC_OS
			// Request permission to access the microphone.
			if (@available(macOS 10.14, *)) {
				switch([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio])
				{
					case AVAuthorizationStatusAuthorized:
					{
						return MicrophonePermissionStateAuthorized;
					}
					case AVAuthorizationStatusNotDetermined:
					{
						return MicrophonePermissionStateNotDetermined;
					}
					case AVAuthorizationStatusDenied:
					case AVAuthorizationStatusRestricted:
						return MicrophonePermissionStateForbidden;
				}
			} else {
				// Fallback on earlier versions
				return MicrophonePermissionStateAuthorized;
			}
#elif RN_PLATFORM_ANDROID
			android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
			//JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();

			JNIEnv* env = nullptr;
			bool isNewEnv = false;

			switch(app->activity->vm->GetEnv((void**)&env, JNI_VERSION_1_8))
			{
				case JNI_OK:
					break;

				case JNI_EDETACHED:
				{
					jint attachresult = app->activity->vm->AttachCurrentThread(&env, nullptr);
					if(attachresult == JNI_ERR)
					{
						RNDebug("error attaching java env to threat");
						return MicrophonePermissionStateNotDetermined;
					}

					isNewEnv = true;
					break;
				}

				case JNI_EVERSION:
					RNDebug("wrong jni version (should be 1.8)");
					return MicrophonePermissionStateNotDetermined;
			}

			jclass activityClass = env->FindClass("android/app/NativeActivity");
			jmethodID getClassLoaderMethod = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
			jobject classLoaderObject = env->CallObjectMethod(app->activity->clazz, getClassLoaderMethod);
			jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
			jmethodID loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

			jstring contextCompatClassName = env->NewStringUTF("androidx.core.content.ContextCompat");
			jclass contextCompatClass = reinterpret_cast<jclass>(env->CallObjectMethod(classLoaderObject, loadClassMethod, contextCompatClassName));
			env->DeleteLocalRef(contextCompatClassName);

			jmethodID checkSelfPermissionMethod = env->GetStaticMethodID(contextCompatClass, "checkSelfPermission", "(Landroid/content/Context;Ljava/lang/String;)I");
			jstring permissionName = env->NewStringUTF("android.permission.RECORD_AUDIO");
			int returnValue = env->CallStaticIntMethod(contextCompatClass, checkSelfPermissionMethod, app->activity->clazz, permissionName);
			env->DeleteLocalRef(permissionName);

			if(isNewEnv)
			{
				app->activity->vm->DetachCurrentThread();
			}

			//Permission not granted
			if(returnValue == -1)
			{
				return MicrophonePermissionStateNotDetermined;
			}
			//Permission granted
			else if(returnValue == 0)
			{
				return MicrophonePermissionStateAuthorized;
			}
#else
		return MicrophonePermissionStateAuthorized;
#endif
	}

	void OpenALWorld::SetInputDevice(String *inputDeviceName)
	{
		if(_inputDevice)
		{
			alcCaptureStop(_inputDevice);
			alcCaptureCloseDevice(_inputDevice);
			_inputDevice = nullptr;
		}
		
		if(GetMicrophonePermissionState() != MicrophonePermissionStateAuthorized) return;
		
		if(inputDeviceName)
		{
			if(inputDeviceName->IsEqual(RNCSTR("default"))) inputDeviceName = nullptr;
			
#if RN_PLATFORM_MAC_OS
			_inputDevice = alcCaptureOpenDevice(inputDeviceName?inputDeviceName->GetUTF8String():nullptr, 48000, AL_FORMAT_MONO16, 1920);
#else
			_inputDevice = alcCaptureOpenDevice(inputDeviceName?inputDeviceName->GetUTF8String():nullptr, 48000, AL_FORMAT_MONO16, 960);
#endif
			
			if(!_inputDevice)
			{
				RNDebug("rayne-openal: Could not open input audio device.");
			}
			else
			{
				alcCaptureStart(_inputDevice);
				_inputBufferTemp = new int16[10240];
			}
		}
	}

	Array *OpenALWorld::GetOutputDeviceNames()
	{
		const char *bytes = static_cast<const char*>(alcGetString(nullptr, ALC_DEVICE_SPECIFIER));
		Array *devices = new Array();
		String *deviceString = String::WithString(bytes, true);
		while(deviceString->GetLength() > 0)
		{
			devices->AddObject(deviceString);
			bytes += deviceString->GetLength() + 1;
			deviceString = String::WithString(bytes, true);
		}
		
		return devices;
	}

	Array *OpenALWorld::GetInputDeviceNames()
	{
		const char *bytes = static_cast<const char*>(alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER));
		Array *devices = new Array();
		String *deviceString = String::WithString(bytes, true);
		while(deviceString->GetLength() > 0)
		{
			devices->AddObject(deviceString);
			bytes += deviceString->GetLength() + 1;
			deviceString = String::WithString(bytes, true);
		}
		
		return devices;
	}
		
	void OpenALWorld::Update(float delta)
	{
		if(_inputDevice && _inputBuffer)
		{
			ALint sampleCount = 0;
			alcGetIntegerv(_inputDevice, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &sampleCount);
			alcCaptureSamples(_inputDevice, (ALCvoid *)_inputBufferTemp, sampleCount);

			for(int i = 0; i < sampleCount; i += 1)
			{
				float value = static_cast<float>(_inputBufferTemp[i])/32768.0f;
				_inputBuffer->PushData(&value, 4);
			}
		}
	}

	void OpenALWorld::SetInputAudioAsset(AudioAsset *bufferAsset)
	{
		SafeRelease(_inputBuffer);
		_inputBuffer = SafeRetain(bufferAsset);
	}
		
	void OpenALWorld::SetListener(OpenALListener *attachment)
	{
		if(_audioListener)
			_audioListener->RemoveFromWorld();
			
		_audioListener = attachment;
			
		if(_audioListener)
			_audioListener->InsertIntoWorld(this);
	}
		
	OpenALSource *OpenALWorld::PlaySound(AudioAsset *resource)
	{
		if(_audioListener)
		{
			OpenALSource *source = new OpenALSource(resource);
			_audioListener->GetParent()->AddChild(source);
			source->SetSelfdestruct(true);
			source->Play();
			return source;
		}
		return nullptr;
	}
}
