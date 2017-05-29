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
		RN_ASSERT(_asset, "No audio asset!");
		RN_ASSERT(_asset->GetBytesPerSample() == 1 || _asset->GetBytesPerSample() == 2 || _asset->GetBytesPerSample() == 4, "Only 8 and 16 and 32 bit audio assets are currently supported.");

		_asset->Retain();
		_totalTime = static_cast<double>(_asset->GetData()->GetLength()) / static_cast<double>(_asset->GetBytesPerSample()) / static_cast<double>(_asset->GetChannels()) / static_cast<double>(_asset->GetSampleRate());
	}
		
	SteamAudioSampler::~SteamAudioSampler()
	{
		_asset->Release();
	}
	
	void SteamAudioSampler::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
	}

	float SteamAudioSampler::GetSample(double time, uint8 channel) const
	{
		if(_isRepeating || _asset->GetType() == AudioAsset::Type::Ringbuffer)
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

		if(upperSamplePosition >= _asset->GetData()->GetLength() / _asset->GetBytesPerSample())
		{
			if(_isRepeating || _asset->GetType() == AudioAsset::Type::Ringbuffer)
				upperSamplePosition %= _asset->GetData()->GetLength() / _asset->GetBytesPerSample();
			else
				upperSamplePosition = lowerSamplePosition;
		}

		float value = 0.0f;
		switch(_asset->GetBytesPerSample())
		{
			case 1:
			{
				int8 *values = static_cast<int8*>(_asset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * interpolationFactor;
				value += values[upperSamplePosition] * (1.0f-interpolationFactor);
				value /= 128.0f;
				break;
			}
			case 2:
			{
				int16 *values = static_cast<int16*>(_asset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * interpolationFactor;
				value += values[upperSamplePosition] * (1.0f - interpolationFactor);
				value /= 32768.0f;
				break;
			}
			case 4:
			{
				float *values = static_cast<float*>(_asset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * interpolationFactor;
				value += values[upperSamplePosition] * (1.0f - interpolationFactor);
				break;
			}

			//TODO: Maybe add 24 and 32 bit support
		}

		return value;
	}
}
