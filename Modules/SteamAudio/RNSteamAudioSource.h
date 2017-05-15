//
//  RNSteamAudioSource.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOSOURCE_H_
#define __RAYNE_STEAMAUDIOSOURCE_H_

#include "RNSteamAudio.h"

namespace RN
{
	class SteamAudioSource : public Object
	{
	public:
		SAAPI SteamAudioSource(AudioAsset *asset);
		SAAPI ~SteamAudioSource() override;
			
		SAAPI void Play();
		SAAPI void Stop();

		SAAPI void SetRepeat(bool repeat);
		SAAPI void SetPitch(float pitch);
		SAAPI void SetGain(float gain);
		SAAPI void SetRange(float min, float max);
		SAAPI void SetSelfdestruct(bool selfdestruct);

		SAAPI void Update(float delta);
		SAAPI float GetSample(uint8 channel) const;
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
			
	private:
		AudioAsset *_asset;
			
		bool _isPlaying;
		bool _isRepeating;
		bool _isSelfdestructing;

		float _gain;
		float _pitch;

		double _currentTime;
		double _totalTime;

		Array *_effects;
			
		RNDeclareMetaAPI(SteamAudioSource, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOSOURCE_H_) */
