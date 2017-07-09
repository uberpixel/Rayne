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
		_asset(nullptr),
		_isRepeating(false)
	{
		SetAudioAsset(asset);
	}
		
	SteamAudioSampler::~SteamAudioSampler()
	{
		_asset->Release();
	}

	void SteamAudioSampler::SetAudioAsset(AudioAsset *asset)
	{
		SafeRelease(_asset);
		if(!asset)
		{
			return;
		}

		RN_ASSERT(asset->GetBytesPerSample() == 1 || asset->GetBytesPerSample() == 2 || asset->GetBytesPerSample() == 4, "Only 8 and 16 and 32 bit audio assets are currently supported.");

		_asset = asset->Retain();
		_totalTime = static_cast<double>(_asset->GetData()->GetLength()) / static_cast<double>(_asset->GetBytesPerSample()) / static_cast<double>(_asset->GetChannels()) / static_cast<double>(_asset->GetSampleRate());
	}
	
	void SteamAudioSampler::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
	}

	float SteamAudioSampler::GetSample(double time, uint8 channel) const
	{
		if(!_asset)
			return 0.0f;

		AudioAsset *tempAsset = _asset->Retain();

		if(_isRepeating || tempAsset->GetType() == AudioAsset::Type::Ringbuffer)
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

		uint8 channelCount = tempAsset->GetChannels();
		uint32 sampleRate = tempAsset->GetSampleRate();
		uint64 lowerSamplePosition = time * sampleRate;
		uint64 upperSamplePosition = time * sampleRate + 1;
		double interpolationFactor = (static_cast<double>(upperSamplePosition) / static_cast<double>(sampleRate) - time) * static_cast<double>(sampleRate);

		lowerSamplePosition = lowerSamplePosition * channelCount + channel;
		upperSamplePosition = upperSamplePosition * channelCount + channel;

		if(upperSamplePosition >= tempAsset->GetData()->GetLength() / tempAsset->GetBytesPerSample())
		{
			if(_isRepeating || tempAsset->GetType() == AudioAsset::Type::Ringbuffer)
				upperSamplePosition %= tempAsset->GetData()->GetLength() / tempAsset->GetBytesPerSample();
			else
				upperSamplePosition = lowerSamplePosition;
		}

		float value = 0.0f;
		switch(tempAsset->GetBytesPerSample())
		{
			case 1:
			{
				int8 *values = static_cast<int8*>(tempAsset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * interpolationFactor;
				value += values[upperSamplePosition] * (1.0f-interpolationFactor);
				value /= 128.0f;
				break;
			}
			case 2:
			{
				int16 *values = static_cast<int16*>(tempAsset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * interpolationFactor;
				value += values[upperSamplePosition] * (1.0f - interpolationFactor);
				value /= 32768.0f;
				break;
			}
			case 4:
			{
				float *values = static_cast<float*>(tempAsset->GetData()->GetBytes());
				value = values[lowerSamplePosition] * interpolationFactor;
				value += values[upperSamplePosition] * (1.0f - interpolationFactor);
				break;
			}

			//TODO: Maybe add 24 and 32 bit support
		}

		tempAsset->Release();

		return value;
	}
}
