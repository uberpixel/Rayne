//
//  RNOculusAudioPlayer.cpp
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusAudioPlayer.h"
#include "RNOculusAudioWorld.h"
#include "RNOculusAudioSampler.h"
#include "RNOculusAudioInternals.h"

namespace RN
{
	RNDefineMeta(OculusAudioPlayer, Object)

		OculusAudioPlayer::OculusAudioPlayer(AudioAsset *asset) :
		_sampler(new OculusAudioSampler(nullptr)),
		_isPlaying(false),
		_isRepeating(false),
		_gain(1.0f),
		_pitch(1.0f),
		_currentTime(0.0f)
	{
		RN_ASSERT(OculusAudioWorld::_instance, "You need to create a OculusAudioWorld before creating audio players!");
		
		SetAudioAsset(asset);
		OculusAudioWorld::_instance->AddAudioPlayer(this);
	}
	
	OculusAudioPlayer::~OculusAudioPlayer()
	{
		if(OculusAudioWorld::_instance)
			OculusAudioWorld::_instance->RemoveAudioPlayer(this);

		_sampler->Release();
	}

	void OculusAudioPlayer::SetAudioAsset(AudioAsset *asset)
	{
		RN_ASSERT(!asset || asset->GetChannels() <= 2, "Currently only mono and stereo files can be played!");
		_sampler->SetAudioAsset(asset);

		if(asset)
		{
			_inputChannels = asset->GetChannels();
		}
	}
		
	void OculusAudioPlayer::SetRepeat(bool repeat)
	{
		_sampler->SetRepeat(repeat);
		_isRepeating = repeat;
	}
	
	void OculusAudioPlayer::SetPitch(float pitch)
	{
		_pitch = pitch;
	}
		
	void OculusAudioPlayer::SetGain(float gain)
	{
		_gain = gain;
	}

	void OculusAudioPlayer::Play()
	{
		_isPlaying = true;
	}

	void OculusAudioPlayer::Stop()
	{
		_isPlaying = false;
	}

	void OculusAudioPlayer::Update(double frameLength, uint32 sampleCount, float **outputBuffer)
	{
		if(!_sampler->GetAsset())
		{
			*outputBuffer = nullptr;
			return;
		}

		double sampleLength = frameLength / static_cast<double>(sampleCount);

		if(_sampler->GetAsset()->GetType() == AudioAsset::Type::Ringbuffer)
		{
			//Buffer for audio data to play
			uint32 assetFrameSamples = std::round(frameLength * _sampler->GetAsset()->GetSampleRate() * _sampler->GetAsset()->GetBytesPerSample());
			if(_sampler->GetAsset()->GetBufferedSize() < assetFrameSamples)
			{
				*outputBuffer = nullptr;
				return;
			}
			else
			{
				//Skip samples if data is written faster than played
				uint32 maxBufferedLength = assetFrameSamples * 4;
				if(_sampler->GetAsset()->GetBufferedSize() > maxBufferedLength)
				{
					uint32 skipBytes = _sampler->GetAsset()->GetBufferedSize() - assetFrameSamples;
					double skipTime = skipBytes / _sampler->GetAsset()->GetBytesPerSample() / static_cast<double>(_sampler->GetAsset()->GetSampleRate());
					_currentTime += skipTime;
					_sampler->GetAsset()->PopData(nullptr, skipBytes);
				}
				
				_sampler->GetAsset()->PopData(nullptr, assetFrameSamples);
			}
		}

		//handle mono input
		if(_inputChannels == 1)
		{
			for (int n = 0; n < sampleCount; n++)
			{
				//TODO: support more output layouts
				for (int i = 0; i < 2; i++)
				{
					OculusAudioWorld::_instance->_sharedSourceOutputFrameData[n * 2 + i] = _sampler->GetSample(_currentTime, 0) * _gain;
				}
				_currentTime += sampleLength * _pitch;
			}
		}

		//handle stereo input
		if(_inputChannels == 2)
		{
			for(int n = 0; n < sampleCount; n++)
			{
				//TODO: support more output layouts
				for(int i = 0; i < 2; i++)
				{
					OculusAudioWorld::_instance->_sharedSourceOutputFrameData[n * 2 + i] = _sampler->GetSample(_currentTime, i) * _gain;
				}
				_currentTime += sampleLength * _pitch;
			}
		}

		*outputBuffer = OculusAudioWorld::_instance->_sharedSourceOutputFrameData;
	}
}
