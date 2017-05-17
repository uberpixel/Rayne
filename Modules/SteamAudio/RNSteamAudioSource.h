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
	class SteamAudioWorld;
	class SteamAudioSampler;
	class SteamAudioSourceInternals;
	class SteamAudioSource : public SceneNode
	{
	public:
		SAAPI SteamAudioSource(AudioAsset *asset, SteamAudioWorld *audioWorld);
		SAAPI ~SteamAudioSource() override;
			
		SAAPI void Play();
		SAAPI void Stop();

		SAAPI void SetRepeat(bool repeat);
		SAAPI void SetPitch(float pitch);
		SAAPI void SetGain(float gain);
		SAAPI void SetRange(float min, float max);
		SAAPI void SetSelfdestruct(bool selfdestruct);

		SAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
			
	private:
		SteamAudioSampler *_sampler;
		SteamAudioSourceInternals *_internals;
			
		bool _isPlaying;
		bool _isRepeating;
		bool _isSelfdestructing;

		float _gain;
		float _pitch;

		double _currentTime;

		Array *_effects;

		static float *_sharedInputBuffer;
		static float *_sharedOutputBuffer;
			
		RNDeclareMetaAPI(SteamAudioSource, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOSOURCE_H_) */
