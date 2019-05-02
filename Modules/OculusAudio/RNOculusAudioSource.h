//
//  RNOculusAudioSource.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudioSOURCE_H_
#define __RAYNE_OculusAudioSOURCE_H_

#include "RNOculusAudio.h"

namespace RN
{
	class OculusAudioSampler;
	struct OculusAudioSourceInternals;
	class OculusAudioSource : public SceneNode
	{
	public:
		friend class OculusAudioWorld;

		OAAPI OculusAudioSource(AudioAsset *asset = nullptr, bool wantsIndirectSound = true);
		OAAPI ~OculusAudioSource() override;
			
		OAAPI void Play();
		OAAPI void Stop();
		OAAPI void Seek(double time);

		OAAPI void SetAudioAsset(AudioAsset *asset);

		OAAPI void SetRepeat(bool repeat);
		OAAPI void SetRadius(float radius);
		OAAPI void SetPitch(float pitch);
		OAAPI void SetGain(float gain);
		OAAPI void SetRange(RN::Vector2 minMaxRange);
//		OAAPI void SetSelfdestruct(bool selfdestruct);
		OAAPI void SetChannel(uint8 channel);
		OAAPI void SetTimeOfFlight(bool tof);

		OAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
		bool HasTimeOfFlight() const { return _hasTimeOfFlight; }
		OAAPI bool HasEnded() const;
		
		RN::Vector2 GetRange() const { return _minMaxRange; }
			
	private:
		uint8 _channel;
		OculusAudioSampler *_sampler;
		OculusAudioSourceInternals *_internals;
		
		bool _wantsIndirectSound;

		bool _isPlaying;
		bool _isRepeating;
//		bool _isSelfdestructing;
		bool _hasTimeOfFlight;

		float _gain;
		float _pitch;
		float _radius;
		
		RN::Vector2 _minMaxRange;

		float _delay;
		float _speed;

		double _currentTime;
			
		RNDeclareMetaAPI(OculusAudioSource, OAAPI)
	};
}

#endif /* defined(__RAYNE_OculusAudioSOURCE_H_) */
