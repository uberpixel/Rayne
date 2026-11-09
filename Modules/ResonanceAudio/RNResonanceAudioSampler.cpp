//
//  RNResonanceAudioSampler.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioSampler.h"
#include "RNResonanceAudioWorld.h"
//#include "RNResonanceAudioEffect.h"

namespace RN
{
	RNDefineMeta(ResonanceAudioSampler, Object)

	ResonanceAudioSampler::ResonanceAudioSampler(AudioAsset *asset) :
		_asset(nullptr)
	{
		SetAudioAsset(asset);
	}

	ResonanceAudioSampler::~ResonanceAudioSampler()
	{
		SafeRelease(_asset);
	}

	void ResonanceAudioSampler::SetAudioAsset(AudioAsset *asset)
	{
		AudioAsset *oldAsset = ExchangeRetainedAsset(SafeRetain(asset));
		SafeRelease(oldAsset);
	}

	AudioAsset *ResonanceAudioSampler::ExchangeRetainedAsset(AudioAsset *asset)
	{
		AudioAsset *oldAsset = std::exchange(_asset, asset);
		_totalTime = 0.0;
		if(asset)
		{
			size_t singleSamplePerChannelSize = asset->GetBytesPerSample() / asset->GetChannels();
			RN_ASSERT(singleSamplePerChannelSize == 1 || singleSamplePerChannelSize == 2 || singleSamplePerChannelSize == 4, "Only 8 and 16 and 32 bit audio assets are currently supported.");

			_totalTime = static_cast<double>(asset->GetData()->GetLength()) / static_cast<double>(asset->GetBytesPerSample()) / static_cast<double>(asset->GetSampleRate());
		}
		return oldAsset;
	}

	double ResonanceAudioSampler::GetTotalTime() const
	{
		return _totalTime;
	}

	float ResonanceAudioSampler::GetSample(double time, uint8 channel, bool isRepeating)
	{
		if(!_asset || !std::isfinite(time))
		{
			return 0.0f;
		}

		if(channel >= _asset->GetChannels())
		{
			RN_DEBUG_ASSERT(false, "Channel out of range");
			return 0.0f;
		}

		if(isRepeating || _asset->GetType() == AudioAsset::Type::Ringbuffer)
		{
			if(_totalTime <= 0.0) return 0.0f;

			if(time < 0.0f)
			{
				time = _totalTime - fmod(-time, _totalTime);
			}
			else if(time >= _totalTime)
			{
				time = fmod(time, _totalTime);
			}
		}
		else
		{
			if(time >= _totalTime || time < 0.0f)
			{
				return 0.0f;
			}
		}

		uint32 sampleRate = _asset->GetSampleRate();
		uint8 channelCount = _asset->GetChannels();
		double fractionalSamplePosition = time * sampleRate;
		int64 sampleFrame = static_cast<int64>(std::floor(fractionalSamplePosition));
		float interpolationFactor = static_cast<float>(fractionalSamplePosition - sampleFrame);
		int64 sampleFrames[4] = {sampleFrame - 1, sampleFrame, sampleFrame + 1, sampleFrame + 2};
		uint64 samplePositions[4];
		int64 frameCount = static_cast<int64>(_asset->GetData()->GetLength() / _asset->GetBytesPerSample());
		if(frameCount <= 0) return 0.0f;

		for(int i = 0; i < 4; i++)
		{
			if(isRepeating || _asset->GetType() == AudioAsset::Type::Ringbuffer)
			{
				sampleFrames[i] %= frameCount;
				if(sampleFrames[i] < 0) sampleFrames[i] += frameCount;
			}
			else
			{
				sampleFrames[i] = std::max<int64>(0, std::min<int64>(sampleFrames[i], frameCount - 1));
			}
			samplePositions[i] = static_cast<uint64>(sampleFrames[i]) * channelCount + channel;
		}

		float valuesToInterpolate[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		switch(_asset->GetBytesPerSample() / channelCount)
		{
			case 1:
			{
				int8 *values = static_cast<int8 *>(_asset->GetData()->GetBytes());
				for(int i = 0; i < 4; i++)
				{
					valuesToInterpolate[i] = values[samplePositions[i]] / 128.0f;
				}
				break;
			}
			case 2:
			{
				int16 *values = static_cast<int16 *>(_asset->GetData()->GetBytes());
				for(int i = 0; i < 4; i++)
				{
					valuesToInterpolate[i] = values[samplePositions[i]] / 32768.0f;
				}
				break;
			}
			case 4:
			{
				float *values = static_cast<float *>(_asset->GetData()->GetBytes());
				for(int i = 0; i < 4; i++)
				{
					valuesToInterpolate[i] = values[samplePositions[i]];
				}
				break;
			}

				//TODO: Maybe add 24 and 32 bit support
		}

		float c0 = valuesToInterpolate[1];
		float c1 = 0.5f * (valuesToInterpolate[2] - valuesToInterpolate[0]);
		float c2 = valuesToInterpolate[0] - (2.5f * valuesToInterpolate[1]) + (2.0f * valuesToInterpolate[2]) - (0.5f * valuesToInterpolate[3]);
		float c3 = (0.5f * (valuesToInterpolate[3] - valuesToInterpolate[0])) + (1.5f * (valuesToInterpolate[1] - valuesToInterpolate[2]));
		float value = (((((c3 * interpolationFactor) + c2) * interpolationFactor) + c1) * interpolationFactor) + c0;

		return value;
	}
} // namespace RN
