//
//  RNOculusAudioPlayer.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudioPLAYER_H_
#define __RAYNE_OculusAudioPLAYER_H_

#include "RNOculusAudio.h"

namespace RN
{
	class OculusAudioSampler;
	class OculusAudioPlayerInternals;
	class OculusAudioPlayer : public Object
	{
	public:
		friend class OculusAudioWorld;

		OAAPI OculusAudioPlayer(AudioAsset *asset = nullptr);
		OAAPI ~OculusAudioPlayer() override;
			
		OAAPI void Play();
		OAAPI void Stop();

		OAAPI void SetAudioAsset(AudioAsset *asset);

		OAAPI void SetRepeat(bool repeat);
		OAAPI void SetPitch(float pitch);
		OAAPI void SetGain(float gain);

		OAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
			
	private:
		OculusAudioSampler *_sampler;

		uint8 _inputChannels;

		bool _isPlaying;
		bool _isRepeating;

		float _gain;
		float _pitch;

		double _currentTime;
			
		RNDeclareMetaAPI(OculusAudioPlayer, OAAPI)
	};
}

#endif /* defined(__RAYNE_OculusAudioPLAYER_H_) */
