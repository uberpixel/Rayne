//
//  RNSteamAudioSource.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioSource.h"

namespace RN
{
	RNDefineMeta(SteamAudioSource, Object)

		SteamAudioSource::SteamAudioSource(AudioAsset *asset) :
		_asset(asset),
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_currentTime(0.0f),
		_gain(1.0f),
		_pitch(1.0f),
		_effects(new Array())
	{
		_asset->Retain();
		_totalTime = _asset->GetData()->GetLength() / (_asset->GetBitsPerSample() / 8) / _asset->GetChannels() / _asset->GetSampleRate();
	}
		
	SteamAudioSource::~SteamAudioSource()
	{
		_asset->Release();
	}
		
	void SteamAudioSource::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
	}
		
	void SteamAudioSource::SetPitch(float pitch)
	{
		_pitch = pitch;
	}
		
	void SteamAudioSource::SetGain(float gain)
	{
		_gain = gain;
	}
		
	void SteamAudioSource::SetRange(float min, float max)
	{

	}
		
	void SteamAudioSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing = selfdestruct;
	}
		
	void SteamAudioSource::Play()
	{
		_isPlaying = true;
	}

	void SteamAudioSource::Stop()
	{
		_isPlaying = false;
	}

	float SteamAudioSource::GetSample(uint8 channel) const
	{
		if (!_isPlaying)
			return 0.0f;

		uint64 samplePosition = _currentTime * _asset->GetSampleRate() *_asset->GetChannels() + channel;

		float sample = 0.0f;
		if(_asset->GetBitsPerSample() == 16)
		{
			int16 *samples = static_cast<int16*>(_asset->GetData()->GetBytes());
			sample = samples[samplePosition];
			sample /= 65536/2;
			sample *= _gain;
		}

		return sample;
	}

	void SteamAudioSource::Update(float delta)
	{
		if(!_isPlaying)
			return;

		_currentTime += delta * _pitch;
		while(_currentTime > _totalTime)
		{
			if(_isRepeating)
			{
				_currentTime -= _totalTime;
			}
			else
			{
				_currentTime = 0.0f;
				_isPlaying = false;

				//TODO: Selfdestruct here if desired?
			}
		}
	}
}
