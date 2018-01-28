//
//  RNSteamAudioSource.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioSource.h"
#include "RNSteamAudioWorld.h"
#include "RNSteamAudioSampler.h"
#include "RNSteamAudioInternals.h"

namespace RN
{
	RNDefineMeta(SteamAudioSource, SceneNode)

	SteamAudioSource::SteamAudioSource(AudioAsset *asset, bool wantsIndirectSound) :
		_channel(0),
		_isPlaying(false),
		_isRepeating(false),
//		_isSelfdestructing(false),
		_hasTimeOfFlight(true),
		_pitch(1.0f),
		_gain(1.0f),
		_currentTime(0.0f),
		_sampler(new SteamAudioSampler(asset)),
		_internals(new SteamAudioSourceInternals()),
		_radius(0.0f),
		_delay(0.0f),
		_speed(0.0f),
		_wantsIndirectSound(wantsIndirectSound)
	{
		RN_ASSERT(SteamAudioWorld::_instance, "You need to create a SteamAudioWorld before creating audio sources!");

		_internals->inputBuffer.format.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		_internals->inputBuffer.format.numSpeakers = 1;
		_internals->inputBuffer.format.channelLayout = IPL_CHANNELLAYOUT_MONO;
		_internals->inputBuffer.format.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		_internals->outputBuffer.format = SteamAudioWorld::_instance->_internals->internalAmbisonicsFormat;

		iplCreatePanningEffect(SteamAudioWorld::_instance->GetBinauralRenderer(), _internals->inputBuffer.format, _internals->outputBuffer.format, &_internals->panningEffect);

		_internals->convolutionEffect = nullptr;
		FinalizeScene();

		SteamAudioWorld::_instance->AddAudioSource(this);
	}
	
	SteamAudioSource::~SteamAudioSource()
	{
		if(SteamAudioWorld::_instance)
			SteamAudioWorld::_instance->RemoveAudioSource(this);

		_sampler->Release();

		if(_internals->panningEffect)
		{
			iplDestroyPanningEffect(&_internals->panningEffect);
		}

		if(_internals->convolutionEffect)
		{
			iplDestroyConvolutionEffect(&_internals->convolutionEffect);
		}

		delete _internals;
	}

	void SteamAudioSource::SetAudioAsset(AudioAsset *asset)
	{
		_sampler->SetAudioAsset(asset);
	}

	void SteamAudioSource::ResetScene()
	{
		if(_internals->convolutionEffect)
		{
			iplDestroyConvolutionEffect(&_internals->convolutionEffect);
			_internals->convolutionEffect = nullptr;
		}
	}

	void SteamAudioSource::FinalizeScene()
	{
		if(_wantsIndirectSound && SteamAudioWorld::_instance->GetEnvironmentalRenderer() && SteamAudioWorld::_instance->_scene)
		{
			iplCreateConvolutionEffect(SteamAudioWorld::_instance->GetEnvironmentalRenderer(), IPLBakedDataIdentifier(), IPL_SIMTYPE_REALTIME, _internals->inputBuffer.format, _internals->outputBuffer.format, &_internals->convolutionEffect); //TODO: Allow baking for static sources instead of doing everything in realtime.
		}
	}
		
	void SteamAudioSource::SetRepeat(bool repeat)
	{
		_sampler->SetRepeat(repeat);
		_isRepeating = repeat;
	}

	void SteamAudioSource::SetRadius(float radius)
	{
		_radius = radius;
	}
	
	void SteamAudioSource::SetPitch(float pitch)
	{
		_pitch = pitch;
	}
		
	void SteamAudioSource::SetGain(float gain)
	{
		_gain = gain;
	}
		
/*	void SteamAudioSource::SetRange(float min, float max)
	{

	}*/
		
/*	void SteamAudioSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing = selfdestruct;
	}*/

	void SteamAudioSource::SetChannel(uint8 channel)
	{
		_channel = channel;
	}

	void SteamAudioSource::SetTimeOfFlight(bool tof)
	{
		_hasTimeOfFlight = tof;
	}

		
	void SteamAudioSource::Play()
	{
		_isPlaying = true;
	}

	void SteamAudioSource::Stop()
	{
		_isPlaying = false;
	}
	
	void SteamAudioSource::Seek(double time)
	{
		_currentTime = time;
	}
	
	bool SteamAudioSource::HasEnded() const
	{
		return (_currentTime >= _sampler->GetTotalTime());
	}

	void SteamAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer)
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

		Vector3 listenerPosition;
		Vector3 listenerForward(0.0f, 0.0f, -1.0f);
		Vector3 listenerUp(0.0f, 1.0f, 0.0f);
		Vector3 sourcePosition = GetWorldPosition();

		SceneNode *listener = SteamAudioWorld::_instance->_listener;
		if(listener)
		{
			listenerPosition = listener->GetWorldPosition();
			listenerForward = listener->GetForward();
			listenerUp = listener->GetUp();
		}

		//Calculate direct sound
		IPLDirectSoundPath directSoundPath = iplGetDirectSoundPath(SteamAudioWorld::_instance->GetEnvironment(),
			IPLVector3{ listenerPosition.x, listenerPosition.y, listenerPosition.z },
			IPLVector3{ listenerForward.x, listenerForward.y, listenerForward.z },
			IPLVector3{ listenerUp.x, listenerUp.y, listenerUp.z },
			IPLVector3{ sourcePosition.x, sourcePosition.y, sourcePosition.z },
			_radius, /*IPL_DIRECTOCCLUSION_NOTRANSMISSION*/ IPL_DIRECTOCCLUSION_NONE, (_radius > 0.001f)?IPL_DIRECTOCCLUSION_VOLUMETRIC : IPL_DIRECTOCCLUSION_RAYCAST);

		//TODO: implement direct sound effect with optional transmission

		if(_hasTimeOfFlight)
		{
			_speed = directSoundPath.propagationDelay - _delay;
			_speed /= sampleCount;
		}
		else
		{
			_speed = 0.0f;
			_delay = 0.0f;
		}
		

		double sampleLength = frameLength / static_cast<double>(sampleCount);
		double localTime = _currentTime;
		for(int i = 0; i < sampleCount; i++)
		{
			SteamAudioWorld::_instance->_sharedSourceInputFrameData[i] = _sampler->GetSample(localTime -_delay, _channel) * directSoundPath.distanceAttenuation * directSoundPath.occlusionFactor * _gain;
			localTime += sampleLength * _pitch;
			_delay += _speed;
		}

		_internals->inputBuffer.numSamples = sampleCount;
		_internals->inputBuffer.interleavedBuffer = SteamAudioWorld::_instance->_sharedSourceInputFrameData;

		_internals->outputBuffer.numSamples = sampleCount;
		_internals->outputBuffer.interleavedBuffer = SteamAudioWorld::_instance->_sharedSourceOutputFrameData;
		*outputBuffer = SteamAudioWorld::_instance->_sharedSourceOutputFrameData;

		iplApplyPanningEffect(_internals->panningEffect, _internals->inputBuffer, directSoundPath.direction, _internals->outputBuffer);

		//Calculate indirect sound
		if(_internals->convolutionEffect && _hasTimeOfFlight)
		{
			localTime = _currentTime;
			for (int i = 0; i < sampleCount; i++)
			{
				SteamAudioWorld::_instance->_sharedSourceInputFrameData[i] = _sampler->GetSample(localTime, _channel) * _gain;
				localTime += sampleLength * _pitch;
			}
			iplSetDryAudioForConvolutionEffect(_internals->convolutionEffect, IPLVector3{ sourcePosition.x, sourcePosition.y, sourcePosition.z }, _internals->inputBuffer);
		}

		_currentTime = localTime;
	}
}
