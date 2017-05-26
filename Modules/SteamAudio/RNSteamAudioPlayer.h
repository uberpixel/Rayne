//
//  RNSteamAudioPlayer.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOPLAYER_H_
#define __RAYNE_STEAMAUDIOPLAYER_H_

#include "RNSteamAudio.h"

namespace RN
{
	class SteamAudioSampler;
	class SteamAudioPlayerInternals;
	class SteamAudioPlayer : public Object
	{
	public:
		friend class SteamAudioWorld;

		SAAPI SteamAudioPlayer(AudioAsset *asset);
		SAAPI ~SteamAudioPlayer() override;
			
		SAAPI void Play();
		SAAPI void Stop();

		SAAPI void SetRepeat(bool repeat);
		SAAPI void SetPitch(float pitch);
		SAAPI void SetGain(float gain);

		SAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
			
	private:
		SteamAudioSampler *_sampler;

		uint8 _inputChannels;

		bool _isPlaying;
		bool _isRepeating;

		bool _isBuffering;

		float _gain;
		float _pitch;

		double _currentTime;
			
		RNDeclareMetaAPI(SteamAudioPlayer, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOPLAYER_H_) */
