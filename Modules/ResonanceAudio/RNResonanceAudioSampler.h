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
		OAAPI ResonanceAudioSampler(AudioAsset *asset = nullptr);
		OAAPI ~ResonanceAudioSampler() override;

		OAAPI void SetAudioAsset(AudioAsset *asset);
		OAAPI void SetRepeat(bool repeat);
		OAAPI float GetSample(double time, uint8 channel);
		OAAPI double GetTotalTime() const;

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
			
		RNDeclareMetaAPI(ResonanceAudioSampler, OAAPI)
	};
}

#endif /* defined(__RAYNE_ResonanceAudioSAMPLER_H_) */
