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
	class SteamAudioSampler;
	class SteamAudioSourceInternals;
	class SteamAudioSource : public SceneNode
	{
	public:
		friend class SteamAudioWorld;

		SAAPI SteamAudioSource(AudioAsset *asset = nullptr, bool wantsIndirectSound = true);
		SAAPI ~SteamAudioSource() override;
			
		SAAPI void Play();
		SAAPI void Stop();

		SAAPI void SetAudioAsset(AudioAsset *asset);

		SAAPI void SetRepeat(bool repeat);
		SAAPI void SetRadius(float radius);
		SAAPI void SetPitch(float pitch);
		SAAPI void SetGain(float gain);
/*		SAAPI void SetRange(float min, float max);
		SAAPI void SetSelfdestruct(bool selfdestruct);*/
		SAAPI void SetChannel(uint8 channel);
		SAAPI void SetTimeOfFlight(bool tof);

		SAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
		bool HasTimeOfFlight() const { return _hasTimeOfFlight; }
			
	private:
		void ResetScene();
		void FinalizeScene();

		uint8 _channel;
		SteamAudioSampler *_sampler;
		SteamAudioSourceInternals *_internals;
		
		bool _wantsIndirectSound;

		bool _isPlaying;
		bool _isRepeating;
//		bool _isSelfdestructing;
		bool _hasTimeOfFlight;

		float _gain;
		float _pitch;
		float _radius;

		float _delay;
		float _speed;

		double _currentTime;
			
		RNDeclareMetaAPI(SteamAudioSource, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOSOURCE_H_) */
