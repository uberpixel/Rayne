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
		BHapticsAndroidWrapper::Initialize();

		//BHapticsAndroidWrapper::GetCurrentDevices();
		
/*		InitialiseSync("com.slindev.grab", "GRAB");
		
		std::vector<bhaptics::DotPoint> points;
		bhaptics::DotPoint point(0, 100);
		points.push_back(point);
		std::vector<bhaptics::PathPoint> points2;
		bhaptics::PathPoint point2(500, 500, 100);
		points2.push_back(point2);

		//SubmitDot("test", bhaptics::PositionType::ForearmL, points, 1000);
		SubmitDot("test2", bhaptics::PositionType::VestFront, points, 1000);
		SubmitPath("path", bhaptics::PositionType::VestBack, points2, 3000);*/
		
		//TurnOffKey("test");
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

	void BHapticsManager::PingAllDevices()
	{
		BHapticsAndroidWrapper::PingAllDevices();
	}

	const Array *BHapticsManager::GetCurrentDevices()
	{
		return BHapticsAndroidWrapper::GetCurrentDevices();
	}

	void BHapticsManager::SubmitDot(const String *key, BHapticsDevicePosition position, const std::vector<BHapticsDotPoint> &points, int duration)
	{
        key->Retain();
        _queue.push_back([key, position, points, duration](){
            BHapticsAndroidWrapper::SubmitDot(key, position, points, duration);
            key->Release();
        });
	}

	void BHapticsManager::RegisterProject(const String *key, const String *filepath)
	{
		String *fileContent = String::WithContentsOfFile(filepath, Encoding::UTF8);
		BHapticsAndroidWrapper::RegisterProject(key, fileContent);
	}

	void BHapticsManager::SubmitProject(const String *key, const String *altKey, float intensity, float duration, float xOffsetAngle, float yOffset)
	{
		key->Retain();
		if(altKey) altKey->Retain();
		_queue.push_back([key, altKey, intensity, duration, xOffsetAngle, yOffset](){
			BHapticsAndroidWrapper::SubmitRegistered(key, altKey, intensity, duration, xOffsetAngle, yOffset);
			
			key->Release();
			if(altKey) altKey->Release();
		});
	}

	void BHapticsManager::TurnOffFeedback(const String *key)
	{
		BHapticsAndroidWrapper::TurnOffFeedback(key);
	}

	void BHapticsManager::TurnOffAllFeedback()
	{
		BHapticsAndroidWrapper::TurnOffAllFeedback();
	}
}
