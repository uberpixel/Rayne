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

		bool HasDevices() const { return _hasDevices; }

		BHAPI void RegisterProject(const String *key, const String *filepath);
        BHAPI void RegisterProjectReflected(const String *key, const String *filepath);
		BHAPI void SubmitProject(const String *key, const String *altKey, float intensity, float duration, float xOffsetAngle, float yOffset);
        BHAPI void SubmitDot(const String *key, BHapticsDevicePosition position, const std::vector<BHapticsDotPoint> &points, int duration);

        BHAPI bool IsFeedbackRegistered(String *key);
        BHAPI bool IsFeedbackPlaying(String *key);
        BHAPI bool IsAnyFeedbackPlaying();

		BHAPI void TurnOffFeedback(const String *key);
		BHAPI void TurnOffAllFeedback();

        BHAPI const Array *GetCurrentDevices();
        BHAPI void PingAllDevices();
		
	private:
		std::vector< std::function<void()> > _queue;
		bool _hasDevices;
			
		RNDeclareMetaAPI(BHapticsManager, BHAPI)
	};
}

#endif /* defined(__RAYNE_BHAPTICS_MANAGER_H_) */
