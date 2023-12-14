//
//  RNResonanceAudioSampler.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioSAMPLER_H_
#define __RAYNE_ResonanceAudioSAMPLER_H_

#include "RNResonanceAudio.h"

namespace RN
{
	class ResonanceAudioWorld;
	class ResonanceAudioSampler : public Object
	{
	public:
		RAAPI ResonanceAudioSampler(AudioAsset *asset = nullptr);
		RAAPI ~ResonanceAudioSampler() override;

		RAAPI void SetAudioAsset(AudioAsset *asset);
		RAAPI void SetRepeat(bool repeat);
		RAAPI float GetSample(double time, uint8 channel);
		RAAPI double GetTotalTime() const;

		AudioAsset *GetAsset()
		{
			LockGuard<Lockable> lock(_lock);
			return _asset;
		}

		//TODO: Implement an optional effects pipeline
			
	private:
		AudioAsset *_asset;
		double _totalTime;
		bool _isRepeating;
		Lockable _lock;
			
		RNDeclareMetaAPI(ResonanceAudioSampler, RAAPI)
	};
}

#endif /* defined(__RAYNE_ResonanceAudioSAMPLER_H_) */
