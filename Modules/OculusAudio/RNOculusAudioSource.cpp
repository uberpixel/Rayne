//
//  RNOculusAudioSource.cpp
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusAudioSource.h"
#include "RNOculusAudioWorld.h"
#include "RNOculusAudioSampler.h"
#include "RNOculusAudioInternals.h"

namespace RN
{
	RNDefineMeta(OculusAudioSource, SceneNode)

	OculusAudioSource::OculusAudioSource(AudioAsset *asset, bool wantsIndirectSound) :
		_channel(0),
		_sampler(new OculusAudioSampler(asset)),
		_oculusAudioSourceIndex(-1),
		_wantsIndirectSound(wantsIndirectSound),
		_isPlaying(false),
		_isRepeating(false),
//		_isSelfdestructing(false),
		_hasTimeOfFlight(true),
		_hasReverb(true),
		_gain(1.0f),
		_pitch(1.0f),
		_radius(0.0f),
		_minMaxRange(RN::Vector2(0.2f, 200.0f)),
		_currentTime(0.0f)
	{
		RN_ASSERT(OculusAudioWorld::_instance, "You need to create a OculusAudioWorld before creating audio sources!");
		OculusAudioWorld::_instance->AddAudioSource(this);
	}
	
	OculusAudioSource::~OculusAudioSource()
	{
		//World retains the source because of this, so if constructor is called, the world doesn't have this source anymore...
/*		if(OculusAudioWorld::_instance)
			OculusAudioWorld::_instance->RemoveAudioSource(this);*/

		_sampler->Release();
	}

	void OculusAudioSource::SetAudioAsset(AudioAsset *asset)
	{
		_sampler->SetAudioAsset(asset);
	}
		
	void OculusAudioSource::SetRepeat(bool repeat)
	{
		_sampler->SetRepeat(repeat);
		_isRepeating = repeat;
	}

	void OculusAudioSource::SetRadius(float radius)
	{
		_radius = radius;
	}
	
	void OculusAudioSource::SetPitch(float pitch)
	{
		_pitch = pitch;
	}
		
	void OculusAudioSource::SetGain(float gain)
	{
		_gain = gain;
	}
		
	void OculusAudioSource::SetRange(RN::Vector2 minMaxRange)
	{
		_minMaxRange = minMaxRange;
	}
		
/*	void OculusAudioSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing = selfdestruct;
	}*/

	void OculusAudioSource::SetChannel(uint8 channel)
	{
		_channel = channel;
	}

	void OculusAudioSource::SetTimeOfFlight(bool tof)
	{
		_hasTimeOfFlight = tof;
	}

		
	void OculusAudioSource::Play()
	{
		_isPlaying = true;
	}

	void OculusAudioSource::Stop()
	{
		_isPlaying = false;
	}
	
	void OculusAudioSource::Seek(double time)
	{
		_currentTime = time;
	}
	
	bool OculusAudioSource::HasEnded() const
	{
		return (_currentTime >= _sampler->GetTotalTime());
	}

	void OculusAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer)
	{
		AudioAsset *asset = _sampler->GetAsset();
		if(!asset)
		{
			*outputBuffer = nullptr;
			return;
		}

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
				uint32 maxBufferedLength = assetFrameSamples * 20;
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

		double sampleLength = frameLength / static_cast<double>(sampleCount);
		double localTime = _currentTime;
		for(int i = 0; i < sampleCount; i++)
		{
			OculusAudioWorld::_instance->_sharedFrameData[i] = _sampler->GetSample(localTime, _channel) * _gain;
			localTime += sampleLength * _pitch;
		}

		_currentTime = localTime;
		*outputBuffer = OculusAudioWorld::_instance->_sharedFrameData;
	}
}
