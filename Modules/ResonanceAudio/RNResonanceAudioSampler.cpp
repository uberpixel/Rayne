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
		_asset(nullptr),
		_isRepeating(false)
	{
		SetAudioAsset(asset);
	}
		
	ResonanceAudioSampler::~ResonanceAudioSampler()
	{
		_asset->Release();
	}

	void ResonanceAudioSampler::SetAudioAsset(AudioAsset *asset)
	{
		LockGuard<Lockable> lock(_lock);

		SafeRelease(_asset);
		if(!asset)
		{
			return;
		}

		RN_ASSERT(asset->GetBytesPerSample() == 1 || asset->GetBytesPerSample() == 2 || asset->GetBytesPerSample() == 4, "Only 8 and 16 and 32 bit audio assets are currently supported.");

		_asset = asset->Retain();
		_totalTime = static_cast<double>(_asset->GetData()->GetLength()) / static_cast<double>(_asset->GetBytesPerSample()) / static_cast<double>(_asset->GetChannels()) / static_cast<double>(_asset->GetSampleRate());
	}
	
	void ResonanceAudioSampler::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
	}
	
	double ResonanceAudioSampler::GetTotalTime() const
	{
		return _totalTime;
	}

	float ResonanceAudioSampler::GetSample(double time, uint8 channel)
	{
        LockGuard<Lockable> lock(_lock);
		if(!_asset)
		{
			return 0.0f;
		}
		
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
		{
			if(time >= _totalTime || time < 0.0f)
			{
				return 0.0f;
			}
		}

		uint32 sampleRate = _asset->GetSampleRate();
		uint8 channelCount = _asset->GetChannels();
		uint64 samplePositions[4];
		samplePositions[0] = time * sampleRate + 2;
		samplePositions[1] = time * sampleRate + 1;
		samplePositions[2] = time * sampleRate - 0;
		samplePositions[3] = time * sampleRate - 1;
		double interpolationFactor = (static_cast<double>(samplePositions[1]) / static_cast<double>(sampleRate) - time) * static_cast<double>(sampleRate);

		uint64 maxSamplePosition = _asset->GetData()->GetLength() / _asset->GetBytesPerSample();
		for(int i = 0; i < 4; i++)
		{
			samplePositions[i] = samplePositions[i] * channelCount + channel;
			
			if(samplePositions[i] >= maxSamplePosition)
			{
				if(_isRepeating || _asset->GetType() == AudioAsset::Type::Ringbuffer)
				{
					samplePositions[i] %= maxSamplePosition;
				}
				else
				{
					samplePositions[i] = maxSamplePosition - (channelCount-channel-1);
				}
			}
			else if(samplePositions[i] < 0)
			{
				if(_isRepeating || _asset->GetType() == AudioAsset::Type::Ringbuffer)
				{
					int64 moduloresult = samplePositions[i] % maxSamplePosition;
					samplePositions[i] = maxSamplePosition + moduloresult;
				}
				else
				{
					samplePositions[i] = channel;
				}
			}
		}

		float valuesToInterpolate[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		switch(_asset->GetBytesPerSample() / channelCount)
		{
			case 1:
			{
				int8 *values = static_cast<int8*>(_asset->GetData()->GetBytes());
				for(int i = 0; i < 4; i++)
				{
					valuesToInterpolate[i] = values[samplePositions[i]] / 128.0f;
				}
				break;
			}
			case 2:
			{
				int16 *values = static_cast<int16*>(_asset->GetData()->GetBytes());
				for(int i = 0; i < 4; i++)
				{
					valuesToInterpolate[i] = values[samplePositions[i]] / 32768.0f;
				}
				break;
			}
			case 4:
			{
				float *values = static_cast<float*>(_asset->GetData()->GetBytes());
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
}
