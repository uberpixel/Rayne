//
//  RNSteamAudioSampler.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioSampler.h"
#include "RNSteamAudioWorld.h"
//#include "RNSteamAudioEffect.h"

namespace RN
{
	RNDefineMeta(SteamAudioSampler, Object)

		SteamAudioSampler::SteamAudioSampler(AudioAsset *asset) :
		_asset(asset),
		_isRepeating(false)
	{
		RN_ASSERT(_asset->GetBitsPerSample() == 8 || _asset->GetBitsPerSample() == 16, "Only 8 and 16 bit audio assets are currently supported.");

		_asset->Retain();
		_totalTime = _asset->GetData()->GetLength() / (_asset->GetBitsPerSample() / 8) / _asset->GetChannels() / _asset->GetSampleRate();
	}
		
	SteamAudioSampler::~SteamAudioSampler()
	{
		_asset->Release();
	}
	
	void SteamAudioSampler::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
	}

	float SteamAudioSampler::GetSample(double time, uint8 channel)
	{
		if(_isRepeating)
		{
			if(time < 0.0f)
			{
				time = _totalTime-fmod(-time, _totalTime);
			}
			else
			{
				time = fmod(time, _totalTime);
			}
		}
		else
			time = std::min(time, _totalTime);

		uint8 channelCount = _asset->GetChannels();
		uint32 sampleRate = _asset->GetSampleRate();
		uint64 lowerSamplePosition = time * sampleRate;
		uint64 upperSamplePosition = time * sampleRate + 1;
		double interpolationFactor = (static_cast<double>(upperSamplePosition) / static_cast<double>(sampleRate) - time) * static_cast<double>(sampleRate);

		lowerSamplePosition = lowerSamplePosition * channelCount + channel;
		upperSamplePosition = upperSamplePosition * channelCount + channel;

		if(upperSamplePosition > _asset->GetData()->GetLength())
		{
			if (_isRepeating)
				upperSamplePosition %= _asset->GetData()->GetLength();
			else
				upperSamplePosition = lowerSamplePosition;
		}

		float value = 0.0f;
		switch(_asset->GetBitsPerSample())
		{
			case 8:
			{
				int8 *values = static_cast<int8*>(_asset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * (1.0f - interpolationFactor);
				value += values[upperSamplePosition] * interpolationFactor;
				value /= 128.0f;
				break;
			}
			case 16:
			{
				int16 *values = static_cast<int16*>(_asset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * (1.0f - interpolationFactor);
				value += values[upperSamplePosition] * interpolationFactor;
				value /= 32768.0f;
			}

			//TODO: Maybe add 24 and 32 bit support
		}

		return value;
	}
}
