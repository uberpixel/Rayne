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
		BHAPI BHapticsManager();
		BHAPI ~BHapticsManager() override;
		
		BHAPI void Update(float delta) override;

		bool HasDevices() const { return _currentDevices && _currentDevices->GetCount() > 0; }

		BHAPI void RegisterProject(const String *key, const String *filepath);
        BHAPI void RegisterProjectReflected(const String *key, const String *filepath);
		BHAPI void SubmitProject(const String *key, const String *altKey, float intensity, float duration, float xOffsetAngle, float yOffset);
        BHAPI void SubmitDot(const String *key, BHapticsDevicePosition position, const std::vector<BHapticsDotPoint> &points, int duration);

        BHAPI bool IsFeedbackRegistered(String *key);
        BHAPI bool IsFeedbackPlaying(String *key);
        BHAPI bool IsAnyFeedbackPlaying();

		BHAPI void TurnOffFeedback(const String *key);
		BHAPI void TurnOffAllFeedback();

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
