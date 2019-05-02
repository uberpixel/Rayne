//
//  RNOculusAudioSampler.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudioSAMPLER_H_
#define __RAYNE_OculusAudioSAMPLER_H_

#include "RNOculusAudio.h"

namespace RN
{
	class OculusAudioWorld;
	class OculusAudioSampler : public Object
	{
	public:
		OAAPI OculusAudioSampler(AudioAsset *asset = nullptr);
		OAAPI ~OculusAudioSampler() override;

		OAAPI void SetAudioAsset(AudioAsset *asset);
		OAAPI void SetRepeat(bool repeat);
		OAAPI float GetSample(double time, uint8 channel);
		OAAPI double GetTotalTime() const;

		OAAPI AudioAsset *GetAsset()
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
			
		RNDeclareMetaAPI(OculusAudioSampler, OAAPI)
	};
}

#endif /* defined(__RAYNE_OculusAudioSAMPLER_H_) */
