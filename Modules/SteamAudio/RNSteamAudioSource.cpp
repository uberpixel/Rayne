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
	float *SteamAudioSource::_sharedOutputBuffer = new float[512 * 15];

	SteamAudioSource::SteamAudioSource(AudioAsset *asset, SteamAudioWorld *audioWorld) :
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_pitch(1.0f),
		_gain(1.0f),
		_currentTime(0.0f),
		_sampler(new SteamAudioSampler(asset)),
		_internals(new SteamAudioSourceInternals())
	{
		_internals->inputBuffer.format.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		_internals->inputBuffer.format.numSpeakers = 1;
		_internals->inputBuffer.format.channelLayout = IPL_CHANNELLAYOUT_MONO;
		_internals->inputBuffer.format.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		_internals->outputBuffer.format.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		_internals->outputBuffer.format.numSpeakers = 4;	//TODO: Experiment with the value, it apparently HAS to be set to something...
		_internals->outputBuffer.format.ambisonicsOrder = 2;	//TODO: Experiment with the value
		_internals->outputBuffer.format.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
		_internals->outputBuffer.format.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		_internals->outputBuffer.format.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		iplCreatePanningEffect(audioWorld->GetBinauralRenderer(), _internals->inputBuffer.format, _internals->outputBuffer.format, &_internals->panningEffect);
	}
		
	SteamAudioSource::~SteamAudioSource()
	{
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

	void SteamAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer)
	{
		double sampleLength = frameLength / static_cast<double>(sampleCount);
		for(int i = 0; i < sampleCount; i++)
		{
			_sharedInputBuffer[i] = _sampler->GetSample(_currentTime, 0);
			_currentTime += sampleLength * _pitch;
		}

		_internals->inputBuffer.numSamples = sampleCount;
		_internals->inputBuffer.interleavedBuffer = _sharedInputBuffer;

		_internals->outputBuffer.numSamples = sampleCount;
		_internals->outputBuffer.interleavedBuffer = _sharedOutputBuffer;
		*outputBuffer = _sharedOutputBuffer;

		iplApplyPanningEffect(_internals->panningEffect, _internals->inputBuffer, IPLVector3{ cosf(_currentTime), 0.0f, -sinf(_currentTime) }, _internals->outputBuffer);
	}
}
