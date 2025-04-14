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

static LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT = nullptr;
static LPALCISRENDERFORMATSUPPORTEDSOFT alcIsRenderFormatSupportedSOFT = nullptr;
static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT = nullptr;

namespace RN
{
	RNDefineMeta(OpenALWorld, SceneAttachment)
	RNDefineMeta(OpenALOutputDevice, Object)

	OpenALWorld *OpenALWorld::_sharedInstance = nullptr;

	OpenALOutputDevice::OpenALOutputDevice(const String *outputDeviceName, bool loopback) : _outputDevice(nullptr), _context(nullptr), _isLoopback(loopback), _missingTime(0.0f), _isManualUpdate(false), _outputBufferTemp(nullptr)
	{
		std::vector<int> attributes;
		attributes.push_back(ALC_HRTF_SOFT);
		attributes.push_back(ALC_TRUE);
		attributes.push_back(ALC_MONO_SOURCES);
		attributes.push_back(512);
		attributes.push_back(ALC_STEREO_SOURCES);
		attributes.push_back(64);
		
		if(loopback)
		{
			if(!alcIsExtensionPresent(NULL, "ALC_SOFT_loopback"))
			{
				RNError("rayne-openal: ALC_SOFT_loopback not supported!");
				return;
			}

			if(!alcLoopbackOpenDeviceSOFT) alcLoopbackOpenDeviceSOFT = reinterpret_cast<LPALCLOOPBACKOPENDEVICESOFT>(alcGetProcAddress(NULL, "alcLoopbackOpenDeviceSOFT"));
			if(!alcIsRenderFormatSupportedSOFT) alcIsRenderFormatSupportedSOFT = reinterpret_cast<LPALCISRENDERFORMATSUPPORTEDSOFT>(alcGetProcAddress(NULL, "alcIsRenderFormatSupportedSOFT"));
			if(!alcRenderSamplesSOFT) alcRenderSamplesSOFT = reinterpret_cast<LPALCRENDERSAMPLESSOFT>(alcGetProcAddress(NULL, "alcRenderSamplesSOFT"));

			_outputDevice = alcLoopbackOpenDeviceSOFT(outputDeviceName ? outputDeviceName->GetUTF8String() : nullptr);
			
			//These are required for contexts with a loopback device!
			attributes.push_back(ALC_FORMAT_CHANNELS_SOFT);
			attributes.push_back(ALC_STEREO_SOFT);
			attributes.push_back(ALC_FORMAT_TYPE_SOFT);
			attributes.push_back(ALC_SHORT_SOFT);
			attributes.push_back(ALC_FREQUENCY);
			attributes.push_back(48000);

			_outputBufferTemp = new int16[2048]; //to fit 1024 stereo samples
		}
		else
		{
			if(outputDeviceName)
				_outputDevice = alcOpenDevice(outputDeviceName->GetUTF8String());
			else
				_outputDevice = alcOpenDevice(nullptr);
			if(!_outputDevice)
			{
				RNError("rayne-openal: Could not open output audio device.");
				return;
			}
		}

		attributes.push_back(0); //End the attributes list with a 0!
		_context = alcCreateContext(_outputDevice, attributes.data());
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

	OpenALOutputDevice::~OpenALOutputDevice()
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(_context);
		alcCloseDevice(_outputDevice);
	}

	void OpenALOutputDevice::MakeCurrent()
	{
		alcMakeContextCurrent(_context);
	}

	size_t OpenALOutputDevice::GetFrameTotalSampleCount(float delta)
	{
		if(!_isLoopback || !_isManualUpdate) return 0;
		
		//Don't lose any time!
		delta += _missingTime;
		size_t sampleCount = delta * 48000;
		_missingTime = delta - sampleCount / 48000.0f;
		
		return sampleCount;
	}

	void OpenALOutputDevice::GetFrameSamples(size_t sampleCount, uint8 *samples)
	{
		if(!_isLoopback || !_isManualUpdate) return;

		size_t offset = 0;
		size_t remainingSamples = sampleCount;
		while(offset < sampleCount)
		{
			size_t requestedSamples = std::min(remainingSamples, size_t(1024));
			if(samples)
			{
				//samples are shorts and interleaved stereo, so each sample consists of two 2byte shorts
				alcRenderSamplesSOFT(_outputDevice, samples + offset * 2 * 2, requestedSamples);
			}
			else
			{
				alcRenderSamplesSOFT(_outputDevice, _outputBufferTemp, requestedSamples);
			}
			offset += requestedSamples;
			remainingSamples -= requestedSamples;
		}
	}

	void OpenALOutputDevice::ProgressContext(float delta)
	{
		if(!_isLoopback || _isManualUpdate) return;
		
		//Don't lose any time!
		delta += _missingTime;
		ALCsizei remainingSamples = delta * 48000;
		_missingTime = delta - remainingSamples / 48000.0f;

		while(remainingSamples > 0)
		{
			ALCsizei sampleCount = std::min(remainingSamples, 1024);
			alcRenderSamplesSOFT(_outputDevice, _outputBufferTemp, sampleCount);
			remainingSamples -= sampleCount;
		}
	}

	void OpenALOutputDevice::SetListener(OpenALListener *attachment)
	{
		if(_audioListener) _audioListener->_owner = nullptr;
		_audioListener = attachment;
		if(_audioListener) _audioListener->_owner = this;
	}

	OpenALSource *OpenALOutputDevice::PlaySound(AudioAsset *resource)
	{
		if(_audioListener)
		{
			OpenALSource *source = new OpenALSource(resource);
			_audioListener->GetParent()->AddChild(source->Autorelease());
			source->SetSelfdestruct(true);
			source->Play();
			return source;
		}
		return nullptr;
	}


	OpenALWorld::OpenALWorld(const String *outputDeviceName) : _outputDevices(new Array()), _inputDevice(nullptr), _inputBuffer(nullptr), _inputBufferTemp(nullptr)
	{
		RN_ASSERT(!_sharedInstance, "There can only be one OpenAL instance at a time!");
		_sharedInstance = this;
		
		OpenALOutputDevice *outputDevice = new OpenALOutputDevice(outputDeviceName);
		_outputDevices->AddObject(outputDevice->Autorelease());
	}

	OpenALWorld::~OpenALWorld()
	{
		if(_inputDevice)
		{
			alcCaptureStop(_inputDevice);
			alcCaptureCloseDevice(_inputDevice);
		}
		
		_outputDevices->Release();

		if(_inputBufferTemp)
		{
			delete[] _inputBufferTemp;
		}
		
		_sharedInstance = nullptr;
	}

	void OpenALWorld::RequestMicrophonePermission()
	{
		MicrophonePermissionState permissionState = GetMicrophonePermissionState();
		if(permissionState == MicrophonePermissionStateNotDetermined)
		{
#if RN_PLATFORM_MAC_OS
			if(@available(macOS 10.14, *))
			{
				[AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
										 completionHandler:^(BOOL granted) {
										 /* if(granted)
					 {
						_inputDevice = alcCaptureOpenDevice(inputDeviceName?inputDeviceName->GetUTF8String():nullptr, 48000, AL_FORMAT_MONO16, 480);
					 }*/
										 }];
			}
#elif RN_PLATFORM_ANDROID
			android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
			JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();

			/*JNIEnv* env = nullptr;
			bool isNewEnv = false;

			switch(app->activity->vm->GetEnv((void**)&env, RN_JNI_VERSION_1_6))
			{
				case JNI_OK:
					break;

				case JNI_EDETACHED:
				{
					jint attachresult = app->activity->vm->AttachCurrentThread(&env, nullptr);
					if(attachresult == JNI_ERR)
					{
						RNDebug("error attaching java env to thread.");
						return;
					}

					isNewEnv = true;
					break;
				}

				case JNI_EVERSION:
					RNDebug("wrong jni version (should be 1.6)");
					return;
			}*/

			//Check for and clear any pending jni exceptions that would prevent the previous code from working
			jboolean flag = env->ExceptionCheck();
			if(flag)
			{
				env->ExceptionDescribe();
				env->ExceptionClear();
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

			/*if(isNewEnv)
			{
				app->activity->vm->DetachCurrentThread();
			}*/
#endif
		}
	}

	OpenALWorld::MicrophonePermissionState OpenALWorld::GetMicrophonePermissionState()
	{
#if RN_PLATFORM_MAC_OS
		// Request permission to access the microphone.
		if(@available(macOS 10.14, *))
		{
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
		}
		else
		{
			// Fallback on earlier versions
			return MicrophonePermissionStateAuthorized;
		}
#elif RN_PLATFORM_ANDROID
		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		JNIEnv *env = Kernel::GetSharedInstance()->GetJNIEnvForRayneMainThread();

		/*JNIEnv* env = nullptr;
			bool isNewEnv = false;

			switch(app->activity->vm->GetEnv((void**)&env, RN_JNI_VERSION_1_6))
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
					RNDebug("wrong jni version (should be 1.6)");
					return MicrophonePermissionStateNotDetermined;
			}*/

		//Check for and clear any pending jni exceptions that would prevent the previous code from working
		jboolean flag = env->ExceptionCheck();
		if(flag)
		{
			env->ExceptionDescribe();
			env->ExceptionClear();
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

		/*if(isNewEnv)
			{
				app->activity->vm->DetachCurrentThread();
			}*/

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
		return MicrophonePermissionStateForbidden;
	}

	void OpenALWorld::SetInputDevice(const String *inputDeviceName)
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
#if RN_PLATFORM_MAC_OS
			_inputDevice = alcCaptureOpenDevice(inputDeviceName && !inputDeviceName->IsEqual(RNCSTR("default")) ? inputDeviceName->GetUTF8String() : nullptr, 48000, AL_FORMAT_MONO16, 1920);
#else
			_inputDevice = alcCaptureOpenDevice(inputDeviceName && !inputDeviceName->IsEqual(RNCSTR("default")) ? inputDeviceName->GetUTF8String() : nullptr, 48000, AL_FORMAT_MONO16, 960);
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
		const char *bytes = static_cast<const char *>(alcGetString(nullptr, ALC_DEVICE_SPECIFIER));
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
		const char *bytes = static_cast<const char *>(alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER));
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
			_inputBuffer->PushData(_inputBufferTemp, sampleCount * 2);
		}
		
		_outputDevices->Enumerate<OpenALOutputDevice>([&](OpenALOutputDevice *device, size_t index, bool &stop){
			device->ProgressContext(delta);
		});
	}

	void OpenALWorld::SetListener(OpenALListener *attachment)
	{
		GetOutputDevice(0)->SetListener(attachment);
	}

	OpenALSource *OpenALWorld::PlaySound(AudioAsset *resource)
	{
		return GetOutputDevice(0)->PlaySound(resource);
	}

	void OpenALWorld::AddOutputDevice(OpenALOutputDevice *device)
	{
		_outputDevices->AddObject(device);
	}

	void OpenALWorld::SetDopplerEffect(float factor, float speedOfSound)
	{
		_outputDevices->Enumerate<OpenALOutputDevice>([&](OpenALOutputDevice *device, size_t index, bool &stop){
			device->MakeCurrent();
			alDopplerFactor(factor);
			alSpeedOfSound(speedOfSound);
		});
	}

	void OpenALWorld::SetInputAudioAsset(AudioAsset *bufferAsset)
	{
		SafeRelease(_inputBuffer);
		_inputBuffer = SafeRetain(bufferAsset);
	}
} // namespace RN
