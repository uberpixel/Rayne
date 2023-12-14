//
//  RNResonanceAudioSource.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioSOURCE_H_
#define __RAYNE_ResonanceAudioSOURCE_H_

#include "RNResonanceAudio.h"

namespace RN
{
	class ResonanceAudioSampler;
	struct ResonanceAudioSourceInternals;
	class ResonanceAudioSource : public SceneNode
	{
	public:
		friend class ResonanceAudioWorld;

		RAAPI ResonanceAudioSource(AudioAsset *asset = nullptr, bool wantsIndirectSound = true);
		RAAPI ~ResonanceAudioSource() override;
			
		RAAPI void Play();
		RAAPI void Stop();
		RAAPI void Seek(double time);

		RAAPI void SetAudioAsset(AudioAsset *asset);

		RAAPI void SetRepeat(bool repeat);
		RAAPI void SetDistanceAttenuation(float attentuation);
		RAAPI void SetPitch(float pitch);
		RAAPI void SetVolume(float volume);
		RAAPI void SetRange(RN::Vector2 minMaxRange);
//		RAAPI void SetSelfdestruct(bool selfdestruct);
		RAAPI void SetChannel(uint8 channel);
		RAAPI void SetTimeOfFlight(bool tof);
		RAAPI void SetReverb(bool reverb);

		RAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
		void Update();
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
		bool HasTimeOfFlight() const { return _hasTimeOfFlight; }
		bool HasReverb() const { return _hasReverb; }
		RAAPI bool HasEnded() const;
		
		RN::Vector2 GetRange() const { return _minMaxRange; }
			
	private:
		uint8 _channel;
		ResonanceAudioSampler *_sampler;
		
		int _sourceID;
		
		bool _wantsIndirectSound;

		bool _isPlaying;
		bool _isRepeating;
//		bool _isSelfdestructing;
		bool _hasTimeOfFlight;
		bool _hasReverb;

		float _gain;
		float _pitch;
		
		RN::Vector2 _minMaxRange;

		double _currentTime;
			
		RNDeclareMetaAPI(ResonanceAudioSource, RAAPI)
	};
}

#endif /* defined(__RAYNE_ResonanceAudioSOURCE_H_) */
