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

		OAAPI ResonanceAudioSource(AudioAsset *asset = nullptr, bool wantsIndirectSound = true);
		OAAPI ~ResonanceAudioSource() override;
			
		OAAPI void Play();
		OAAPI void Stop();
		OAAPI void Seek(double time);

		OAAPI void SetAudioAsset(AudioAsset *asset);

		OAAPI void SetRepeat(bool repeat);
		OAAPI void SetDistanceAttenuation(float attentuation);
		OAAPI void SetPitch(float pitch);
		OAAPI void SetVolume(float volume);
		OAAPI void SetRange(RN::Vector2 minMaxRange);
//		OAAPI void SetSelfdestruct(bool selfdestruct);
		OAAPI void SetChannel(uint8 channel);
		OAAPI void SetTimeOfFlight(bool tof);
		OAAPI void SetReverb(bool reverb);

		OAAPI void Update(double frameLength, uint32 sampleCount, float **outputBuffer);
		void Update();
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
			
		bool IsPlaying() const { return _isPlaying; }
		bool IsRepeating() const { return _isRepeating; }
		bool HasTimeOfFlight() const { return _hasTimeOfFlight; }
		bool HasReverb() const { return _hasReverb; }
		OAAPI bool HasEnded() const;
		
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
			
		RNDeclareMetaAPI(ResonanceAudioSource, OAAPI)
	};
}

#endif /* defined(__RAYNE_ResonanceAudioSOURCE_H_) */
