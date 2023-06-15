//
//  RNBHapticsManager.cpp
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBHapticsManager.h"

#if RN_PLATFORM_WINDOWS
	#include "HapticLibrary.h"
	#include "model.h"
#elif RN_PLATFORM_ANDROID
	#include "RNBHapticsAndroidWrapper.h"
#endif

namespace RN
{
	RNDefineMeta(BHapticsManager, SceneAttachment)
	
	BHapticsManager::BHapticsManager(const String *applicationID, const String *apiKey, const String *defaultConfig, bool requestPermission) : _currentDevices(nullptr), _wantsDeviceUpdate(true)
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::Initialize(applicationID, apiKey, defaultConfig, requestPermission);
#elif RN_PLATFORM_WINDOWS
        //Initialise("com.slindev.grab", "GRAB");
#endif
	}
		
	BHapticsManager::~BHapticsManager()
	{
#if RN_PLATFORM_WINDOWS
		//Destroy();
#endif
	}

	void BHapticsManager::Update(float delta)
	{
	    if(_wantsDeviceUpdate)
	    {
			const Array *currentDevices = nullptr;
#if RN_PLATFORM_ANDROID
			currentDevices = BHapticsAndroidWrapper::GetCurrentDevices();
#elif RN_PLATFORM_WINDOWS
#endif
            if(currentDevices)
            {
				SafeRelease(_currentDevices);
				_currentDevices = currentDevices->Retain();
            }
			
			_wantsDeviceUpdate = false;
        }

		for(const auto call : _queue)
		{
			call();
		}
		_queue.clear();
	}

	void BHapticsManager::Play(const String *eventName, float intensity, float duration, float xOffsetAngle, float yOffset)
	{
#if RN_PLATFORM_ANDROID
		eventName->Retain();
		_queue.push_back([eventName, intensity, duration, xOffsetAngle, yOffset](){
			BHapticsAndroidWrapper::Play(eventName, intensity, duration, xOffsetAngle, yOffset);
			eventName->Release();
		});
#elif RN_PLATFORM_WINDOWS
#endif
	}

/*	void BHapticsManager::SubmitDot(const String *key, BHapticsDevicePosition position, const std::vector<BHapticsDotPoint> &points, int duration)
	{
#if RN_PLATFORM_ANDROID
		key->Retain();
		_queue.push_back([key, position, points, duration](){
			BHapticsAndroidWrapper::SubmitDot(key, position, points, duration);
			key->Release();
		});
#elif RN_PLATFORM_WINDOWS
		//SubmitDot(key->GetUTF8String(), key, points, 1000);
#endif
	}*/

	bool BHapticsManager::IsPlayingByEventName(const String *eventName)
	{
#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsPlayingByEventName(eventName);
#elif RN_PLATFORM_WINDOWS
#endif
		return false;
	}

	bool BHapticsManager::IsPlaying()
	{
#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsPlaying();
#elif RN_PLATFORM_WINDOWS
#endif
		return false;
	}

	void BHapticsManager::StopByEventName(const String *eventName)
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::StopByEventName(eventName);
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::Stop()
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::Stop();
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::UpdateCurrentDevices()
	{
		_wantsDeviceUpdate = true;
	}

	const Array *BHapticsManager::GetCurrentDevices() const
	{
		return _currentDevices;
	}

	void BHapticsManager::PingAllDevices()
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::PingAll();
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::PingDevice(BHapticsDevicePosition position)
	{
		if(!_currentDevices) return;
		
		_currentDevices->Enumerate<BHapticsDevice>([&](BHapticsDevice *device, size_t index, bool &stop){
			if(device->position == position && device->address)
			{
				device->Retain();
				_queue.push_back([device](){
#if RN_PLATFORM_ANDROID
					BHapticsAndroidWrapper::Ping(device->address);
#endif
					device->Release();
				});
				stop = true;
			}
		});
	}
}
