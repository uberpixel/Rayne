//
//  RNBHapticsManager.cpp
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBHapticsManager.h"

#if RN_PLATFORM_WINDOWS
	#include "BhapticsCPP.h"
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
		registryAndInit(apiKey->GetUTF8String(), applicationID->GetUTF8String(), defaultConfig->GetUTF8String());

		if(isPlayerInstalled())
		{
			if(!isPlayerRunning())
			{
				//UE_LOG(BhapticsPlugin, Log, TEXT("Player is not running and TryLaunch"));
				launchPlayer(true);
			}
		}
#endif
	}
		
	BHapticsManager::~BHapticsManager()
	{
#if RN_PLATFORM_WINDOWS
		wsClose();
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
			std::string deviceStr = getDeviceInfoJson();

			if(wsIsConnected() && deviceStr.length() > 0)
			{
				const Array *jsonDevices = JSONSerialization::ObjectFromString<RN::Array>(RNSTR(deviceStr));
				if (jsonDevices && jsonDevices->GetCount() > 0)
				{
					Array *devices = new RN::Array(jsonDevices->GetCount());
					jsonDevices->Enumerate<Dictionary>([&](Dictionary *dict, size_t index, bool &stop) {
						BHapticsDevice *device = new BHapticsDevice();

						device->deviceName = SafeRetain(dict->GetObjectForKey<String>(RNCSTR("deviceName")));
						device->address = SafeRetain(dict->GetObjectForKey<String>(RNCSTR("address")));

						const Number *positionNumber = dict->GetObjectForKey<Number>(RNCSTR("position"));
						device->position = static_cast<BHapticsDevicePosition>(positionNumber->GetUint8Value());

						const Number *isConnectedNumber = dict->GetObjectForKey<Number>(RNCSTR("connected"));
						device->isConnected = isConnectedNumber->GetBoolValue();

						const Number *isPairedNumber = dict->GetObjectForKey<Number>(RNCSTR("paired"));
						device->isPaired = isPairedNumber->GetBoolValue();

						devices->AddObject(device);
					});

					currentDevices = devices->Autorelease();
				}
			}
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
		if(!_currentDevices) return;

		eventName->Retain();
		_queue.push_back([eventName, intensity, duration, xOffsetAngle, yOffset](){
#if RN_PLATFORM_ANDROID
			BHapticsAndroidWrapper::Play(eventName, intensity, duration, xOffsetAngle, yOffset);
#elif RN_PLATFORM_WINDOWS
			playPosParam(eventName->GetUTF8String(), 0, intensity, duration, xOffsetAngle, yOffset);
#endif

			eventName->Release();
		});
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
		if(!_currentDevices) return false;

#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsPlayingByEventName(eventName);
#elif RN_PLATFORM_WINDOWS
		return isPlayingByEventId(eventName->GetUTF8String());
#endif
		return false;
	}

	bool BHapticsManager::IsPlaying()
	{
		if(!_currentDevices) return false;

#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsPlaying();
#elif RN_PLATFORM_WINDOWS
		return isPlaying();
#endif
		return false;
	}

	void BHapticsManager::StopByEventName(const String *eventName)
	{
		if(!_currentDevices) return;

#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::StopByEventName(eventName);
#elif RN_PLATFORM_WINDOWS
		stopByEventId(eventName->GetUTF8String());
#endif
	}

	void BHapticsManager::Stop()
	{
		if(!_currentDevices) return;

#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::Stop();
#elif RN_PLATFORM_WINDOWS
		stopAll();
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
		if(!_currentDevices) return;

#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::PingAll();
#elif RN_PLATFORM_WINDOWS
		pingAll();
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
#elif RN_PLATFORM_WINDOWS
					ping(device->address->GetUTF8String());
#endif
					device->Release();
				});
				stop = true;
			}
		});
	}
}
