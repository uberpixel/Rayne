//
//  RNBHapticsManager.h
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BHAPTICS_MANAGER_H_
#define __RAYNE_BHAPTICS_MANAGER_H_

#include "RNBHaptics.h"
#include "RNBHapticsTypes.h"

namespace RN
{
	class BHapticsManager : public SceneAttachment
	{
	public:
		BHAPI BHapticsManager(const String *applicationID, const String *apiKey, const String *defaultConfig, bool requestPermission);
		BHAPI ~BHapticsManager() override;
		
		BHAPI void Update(float delta) override;

		bool HasDevices() const { return _currentDevices && _currentDevices->GetCount() > 0; }

		BHAPI void Play(const String *eventName, float intensity, float duration, float xOffsetAngle, float yOffset);
        //BHAPI void SubmitDot(const String *key, BHapticsDevicePosition position, const std::vector<BHapticsDotPoint> &points, int duration);

        BHAPI bool IsPlayingByEventName(const String *key);
        BHAPI bool IsPlaying();

		BHAPI void StopByEventName(const String *eventName);
		BHAPI void Stop();

		BHAPI void UpdateCurrentDevices();
        BHAPI const Array *GetCurrentDevices() const;
        BHAPI void PingAllDevices();
		BHAPI void PingDevice(BHapticsDevicePosition position);
		
	private:
		std::vector< std::function<void()> > _queue;
		const Array *_currentDevices;
		bool _wantsDeviceUpdate;
			
		RNDeclareMetaAPI(BHapticsManager, BHAPI)
	};
}

#endif /* defined(__RAYNE_BHAPTICS_MANAGER_H_) */
