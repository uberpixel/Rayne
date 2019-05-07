//
//  RNOculusAudioSystem.cpp
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusAudioSystem.h"
#include "RNOculusAudioInternals.h"

namespace RN
{
	RNDefineMeta(OculusAudioDevice, Object)
	RNDefineMeta(OculusAudioSystem, Object)

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	OculusAudioSystem::OculusAudioSystem(uint32 sampleRate, uint32 frameSize) : _frameSize(frameSize), _sampleRate(sampleRate)
	{

	}
		
	OculusAudioSystem::~OculusAudioSystem()
	{
		
	}
	
	OculusAudioSystem *OculusAudioSystem::WithInfo(uint32 sampleRate, uint32 frameSize)
	{
		OculusAudioSystem *audioSystem = nullptr;
		
#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
		audioSystem = new OculusAudioSystemRtAudio(sampleRate, frameSize);
#elif RN_PLATFORM_ANDROID
		
#endif
		
		RN_ASSERT(audioSystem, "Couldn't create audio system, platform not supported?");
		return audioSystem->Autorelease();
	}

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
	
	RNDefineMeta(OculusAudioSystemRtAudio, OculusAudioSystem)
	
	OculusAudioSystemRtAudio::OculusAudioSystemRtAudio(uint32 sampleRate, uint32 frameSize) : OculusAudioSystem(sampleRate, frameSize), _internals(new OculusAudioSystemRtAudioInternals()), _outputDevice(nullptr), _inputDevice(nullptr)
	{
		
	}
	
	OculusAudioSystemRtAudio::~OculusAudioSystemRtAudio()
	{
		delete _internals;
	}
	
	int OculusAudioSystemRtAudio::AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int frameSize, double streamTime, RtAudioStreamStatus status, void *userData)
	{
		RN_ASSERT(userData, "User data not set in RtAudio callback. This should never happen");
		OculusAudioSystemRtAudio *instance = static_cast<OculusAudioSystemRtAudio*>(userData);
		
		if(instance->_audioCallback)
		{
			instance->_audioCallback(outputBuffer, inputBuffer, frameSize, status);
		}
		
		return 0;
	}
	
	void OculusAudioSystemRtAudio::SetOutputDevice(OculusAudioDevice *outputDevice)
	{
		RN_ASSERT(!outputDevice || outputDevice->type == OculusAudioDevice::Type::Output, "Not an output device!");
		
		SafeRelease(_outputDevice);
		_outputDevice = outputDevice;
		SafeRetain(_outputDevice);
		
		UpdateStreamSettings();

		RNInfo("Using audio output device: " << _outputDevice->name);
	}

	void OculusAudioSystemRtAudio::SetInputDevice(OculusAudioDevice *inputDevice)
	{
		RN_ASSERT(!inputDevice || inputDevice->type == OculusAudioDevice::Type::Input, "Not an input device!");
		
		SafeRelease(_inputDevice);
		_inputDevice = inputDevice;
		SafeRetain(_inputDevice);
		
		UpdateStreamSettings();
		
		RNInfo("Using audio input device: " << _inputDevice->name);
	}
	
	void OculusAudioSystemRtAudio::UpdateStreamSettings()
	{
		if(_internals->rtAudioContext.isStreamOpen())
			_internals->rtAudioContext.closeStream();
		
		RtAudio::StreamParameters outputParameters;
		RtAudio::StreamParameters *outputParametersPtr = nullptr;
		
		RtAudio::StreamParameters inputParameters;
		RtAudio::StreamParameters *inputParametersPtr = nullptr;
		
		if(_outputDevice)
		{
			outputParameters.deviceId = _outputDevice->index;
			outputParameters.nChannels = 2;
			outputParameters.firstChannel = 0;
			
			outputParametersPtr = &outputParameters;
		}
		
		if(_inputDevice)
		{
			inputParameters.deviceId = _inputDevice->index;
			inputParameters.nChannels = 1;
			inputParameters.firstChannel = 0;
			
			inputParametersPtr = &inputParameters;
		}
		
		try
		{
			_internals->rtAudioContext.openStream(outputParametersPtr, inputParametersPtr, RTAUDIO_FLOAT32, _sampleRate, &_frameSize, &AudioCallback, this);
			_internals->rtAudioContext.startStream();
		}
		catch(RtAudioError& e)
		{
			e.printMessage();
			return;
		}
	}

	Array *OculusAudioSystemRtAudio::GetDevices()
	{
		unsigned int devices = _internals->rtAudioContext.getDeviceCount();
		
		Array *deviceArray = new Array();
		
		RtAudio::DeviceInfo info;
		for(unsigned int i = 0; i < devices; i++)
		{
			info = _internals->rtAudioContext.getDeviceInfo(i);
			if(info.probed == true)
			{
				if(info.outputChannels > 0)
				{
					OculusAudioDevice *outputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Output, i, info.name, info.isDefaultOutput);
					deviceArray->AddObject(outputDevice->Autorelease());
				}
				
				if(info.inputChannels > 0)
				{
					OculusAudioDevice *inputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Input, i, info.name, info.isDefaultInput);
					deviceArray->AddObject(inputDevice->Autorelease());
				}
			}
		}
		
		return deviceArray->Autorelease();
	}

	OculusAudioDevice *OculusAudioSystemRtAudio::GetDefaultOutputDevice()
	{
		try
		{
			unsigned int defaultDeviceIndex = _internals->rtAudioContext.getDefaultOutputDevice();
			RtAudio::DeviceInfo info = _internals->rtAudioContext.getDeviceInfo(defaultDeviceIndex);
			OculusAudioDevice *outputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Output, defaultDeviceIndex, info.name, info.isDefaultOutput);
			return outputDevice->Autorelease();
		}
		catch(RtAudioError error)
		{
			RNDebug(error.getMessage());
		}
		
		RNDebug("No default audio output device found.");
		return nullptr;
	}

	OculusAudioDevice *OculusAudioSystemRtAudio::GetDefaultInputDevice()
	{
		try
		{
			unsigned int defaultDeviceIndex = _internals->rtAudioContext.getDefaultInputDevice();
			RtAudio::DeviceInfo info = _internals->rtAudioContext.getDeviceInfo(defaultDeviceIndex);
			OculusAudioDevice *inputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Output, defaultDeviceIndex, info.name, info.isDefaultOutput);
			return inputDevice->Autorelease();
		}
		catch(RtAudioError error)
		{
			RNDebug(error.getMessage());
		}
		
		RNDebug("No default audio output device found.");
		return nullptr;
	}
	
#elif RN_PLATFORM_ANDROID
	
#endif
}
