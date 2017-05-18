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

	//TODO: Don't hardcode the size of these...
	float *SteamAudioSource::_sharedInputBuffer = new float[512];
	float *SteamAudioSource::_sharedOutputBuffer = new float[512 * 16];

	SteamAudioSource::SteamAudioSource(AudioAsset *asset, bool hasIndirectSound) :
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_pitch(1.0f),
		_gain(1.0f),
		_currentTime(0.0f),
		_sampler(new SteamAudioSampler(asset)),
		_internals(new SteamAudioSourceInternals()),
		_radius(0.025f),
		_delay(0.0f),
		_speed(0.0f)
	{
		RN_ASSERT(SteamAudioWorld::_instance, "You need to create a SteamAudioWorld before creating audio sources!");

		_internals->inputBuffer.format.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		_internals->inputBuffer.format.numSpeakers = 1;
		_internals->inputBuffer.format.channelLayout = IPL_CHANNELLAYOUT_MONO;
		_internals->inputBuffer.format.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		_internals->outputBuffer.format.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		_internals->outputBuffer.format.numSpeakers = 16;	//TODO: Experiment with the value, it apparently HAS to be set to something...
		_internals->outputBuffer.format.ambisonicsOrder = 3;	//TODO: Experiment with the value
		_internals->outputBuffer.format.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
		_internals->outputBuffer.format.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		_internals->outputBuffer.format.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		iplCreatePanningEffect(SteamAudioWorld::_instance->GetBinauralRenderer(), _internals->inputBuffer.format, _internals->outputBuffer.format, &_internals->panningEffect);

		_internals->convolutionEffect = nullptr;
		if(hasIndirectSound)
		{
			iplCreateConvolutionEffect(SteamAudioWorld::_instance->GetEnvironmentalRenderer(), "", IPL_SIMTYPE_REALTIME, _internals->inputBuffer.format, _internals->outputBuffer.format, &_internals->convolutionEffect); //TODO: Allow baking for static sources instead of doing everything in realtime.
		}

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

	void SteamAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer)
	{
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

		IPLDirectSoundPath directSoundPath = iplGetDirectSoundPath(SteamAudioWorld::_instance->_environmentalRenderer,
			IPLVector3{ listenerPosition.x, listenerPosition.y, listenerPosition.z },
			IPLVector3{ listenerForward.x, listenerForward.y, listenerForward.z },
			IPLVector3{ listenerUp.x, listenerUp.y, listenerUp.z },
			IPLVector3{ sourcePosition.x, sourcePosition.y, sourcePosition.z },
			_radius, IPL_DIRECTOCCLUSION_VOLUMETRIC);	//TODO: Use IPL_DIRECTOCCLUSION_VOLUMETRIC once there is an environment

		//Maybe use smoothing?
		//float tempSpeed = directSoundPath.propagationDelay - _delay;
		//tempSpeed /= sampleCount;

		_speed = directSoundPath.propagationDelay - _delay;
		_speed /= sampleCount;

		double sampleLength = frameLength / static_cast<double>(sampleCount);
		double localTime = _currentTime;
		for(int i = 0; i < sampleCount; i++)
		{
			//_speed *= 0.9f;
			//_speed += tempSpeed*0.1f;
			_sharedInputBuffer[i] = _sampler->GetSample(localTime -_delay, 0) *directSoundPath.distanceAttenuation * directSoundPath.occlusionFactor * _gain;
			localTime += sampleLength * _pitch;
			_delay += _speed;
		}

		_internals->inputBuffer.numSamples = sampleCount;
		_internals->inputBuffer.interleavedBuffer = _sharedInputBuffer;

		_internals->outputBuffer.numSamples = sampleCount;
		_internals->outputBuffer.interleavedBuffer = _sharedOutputBuffer;
		*outputBuffer = _sharedOutputBuffer;

		iplApplyPanningEffect(_internals->panningEffect, _internals->inputBuffer, directSoundPath.direction, _internals->outputBuffer);

		if(_internals->convolutionEffect)
		{
			localTime = _currentTime;
			for (int i = 0; i < sampleCount; i++)
			{
				_sharedInputBuffer[i] = _sampler->GetSample(localTime, 0) * _gain;
				localTime += sampleLength * _pitch;
			}
			iplSetDryAudioForConvolutionEffect(_internals->convolutionEffect, IPLVector3{ sourcePosition.x, sourcePosition.y, sourcePosition.z }, _internals->inputBuffer);
		}

		_currentTime = localTime;
	}
}
