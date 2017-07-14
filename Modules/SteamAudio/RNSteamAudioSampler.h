//
//  RNSteamAudioSampler.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOSAMPLER_H_
#define __RAYNE_STEAMAUDIOSAMPLER_H_

#include "RNSteamAudio.h"

namespace RN
{
	class SteamAudioWorld;
	class SteamAudioSampler : public Object
	{
	public:
		SAAPI SteamAudioSampler(AudioAsset *asset = nullptr);
		SAAPI ~SteamAudioSampler() override;

		SAAPI void SetAudioAsset(AudioAsset *asset);
		SAAPI void SetRepeat(bool repeat);
		SAAPI float GetSample(double time, uint8 channel);

		SAAPI AudioAsset *GetAsset()
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
			
		RNDeclareMetaAPI(SteamAudioSampler, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOSAMPLER_H_) */
