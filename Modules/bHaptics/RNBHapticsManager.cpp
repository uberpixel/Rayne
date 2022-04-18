//
//  RNBHapticsManager.cpp
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBHapticsManager.h"

#include "RNBHapticsAndroidWrapper.h"

#include "HapticLibrary.h"
#include "model.h"

namespace RN
{
	RNDefineMeta(BHapticsManager, SceneAttachment)
	
	BHapticsManager::BHapticsManager() : _hasDevices(false)
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::Initialize();
#elif RN_PLATFORM_WINDOWS
        Initialise("com.slindev.grab", "GRAB");
#endif
	}
		
	BHapticsManager::~BHapticsManager()
	{
		Destroy();
	}

	void BHapticsManager::Update(float delta)
	{
	    if(!_hasDevices)
	    {
            const Array *currentDevices = GetCurrentDevices();
            if(currentDevices && currentDevices->GetCount() > 0)
            {
                _hasDevices = true;
            }
        }

		for(const auto call : _queue)
		{
			call();
		}
		_queue.clear();
	}

	void BHapticsManager::RegisterProject(const String *key, const String *filepath)
	{
#if RN_PLATFORM_ANDROID
		String *fileContent = String::WithContentsOfFile(filepath, Encoding::UTF8);
		BHapticsAndroidWrapper::RegisterProject(key, fileContent);
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::RegisterProjectReflected(const String *key, const String *filepath)
	{
#if RN_PLATFORM_ANDROID
		String *fileContent = String::WithContentsOfFile(filepath, Encoding::UTF8);
		BHapticsAndroidWrapper::RegisterProjectReflected(key, fileContent);
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::SubmitProject(const String *key, const String *altKey, float intensity, float duration, float xOffsetAngle, float yOffset)
	{
#if RN_PLATFORM_ANDROID
		key->Retain();
		if(altKey) altKey->Retain();
		_queue.push_back([key, altKey, intensity, duration, xOffsetAngle, yOffset](){
			BHapticsAndroidWrapper::SubmitRegistered(key, altKey, intensity, duration, xOffsetAngle, yOffset);
			
			key->Release();
			if(altKey) altKey->Release();
		});
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::SubmitDot(const String *key, BHapticsDevicePosition position, const std::vector<BHapticsDotPoint> &points, int duration)
	{
#if RN_PLATFORM_ANDROID
		key->Retain();
		_queue.push_back([key, position, points, duration](){
			BHapticsAndroidWrapper::SubmitDot(key, position, points, duration);
			key->Release();
		});
#elif RN_PLATFORM_WINDOWS
		SubmitDot(key->GetUTF8String(), key, points, 1000);
#endif
	}

	bool BHapticsManager::IsFeedbackRegistered(String *key)
	{
#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsFeedbackRegistered(key);
#elif RN_PLATFORM_WINDOWS
#endif
		return false;
	}

	bool BHapticsManager::IsFeedbackPlaying(String *key)
	{
#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsFeedbackPlaying(key);
#elif RN_PLATFORM_WINDOWS
#endif
		return false;
	}

	bool BHapticsManager::IsAnyFeedbackPlaying()
	{
#if RN_PLATFORM_ANDROID
		return BHapticsAndroidWrapper::IsAnyFeedbackPlaying();
#elif RN_PLATFORM_WINDOWS
#endif
		return false;
	}

	void BHapticsManager::TurnOffFeedback(const String *key)
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::TurnOffFeedback(key);
#elif RN_PLATFORM_WINDOWS
#endif
	}

	void BHapticsManager::TurnOffAllFeedback()
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::TurnOffAllFeedback();
#elif RN_PLATFORM_WINDOWS
#endif
	}

	const Array *BHapticsManager::GetCurrentDevices()
	{
		return BHapticsAndroidWrapper::GetCurrentDevices();
	}

	void BHapticsManager::PingAllDevices()
	{
#if RN_PLATFORM_ANDROID
		BHapticsAndroidWrapper::PingAllDevices();
#elif RN_PLATFORM_WINDOWS
#endif
	}
}
