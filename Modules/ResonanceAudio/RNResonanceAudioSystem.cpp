//
//  RNResonanceAudioSystem.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioSystem.h"
#include "RNResonanceAudioInternals.h"

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace RN
{
	RNDefineMeta(ResonanceAudioDevice, Object)
	RNDefineMeta(ResonanceAudioSystem, Object)

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	ResonanceAudioSystem::ResonanceAudioSystem(uint32 sampleRate, uint32 frameSize, uint8 channelCount) : _frameSize(frameSize), _sampleRate(sampleRate), _channelCount(channelCount)
	{

	}
		
	ResonanceAudioSystem::~ResonanceAudioSystem()
	{
		
	}
	
	ResonanceAudioSystem *ResonanceAudioSystem::WithInfo(uint32 sampleRate, uint32 frameSize, uint8 channelCount)
	{
		ResonanceAudioSystem *audioSystem = nullptr;
		audioSystem = new ResonanceAudioSystemMiniAudio(sampleRate, frameSize, channelCount);
		
		RN_ASSERT(audioSystem, "Couldn't create audio system, platform not supported?");
		return audioSystem->Autorelease();
	}


	RNDefineMeta(ResonanceAudioDeviceMiniAudio, ResonanceAudioDevice)
	RNDefineMeta(ResonanceAudioSystemMiniAudio, ResonanceAudioSystem)

	ResonanceAudioDeviceMiniAudio::~ResonanceAudioDeviceMiniAudio()
	{
		ma_device_id *typedDeviceID = static_cast<ma_device_id*>(deviceID);
		if(typedDeviceID) delete typedDeviceID;
	}
	
	ResonanceAudioSystemMiniAudio::ResonanceAudioSystemMiniAudio(uint32 sampleRate, uint32 frameSize, uint8 channelCount) : ResonanceAudioSystem(sampleRate, frameSize, channelCount), _internals(new ResonanceAudioSystemMiniAudioInternals()), _outputDevice(nullptr), _inputDevice(nullptr)
	{
		if(ma_context_init(NULL, 0, NULL, &_internals->context) != MA_SUCCESS)
		{
			// Error.
		}
	}
	
	ResonanceAudioSystemMiniAudio::~ResonanceAudioSystemMiniAudio()
	{
		if(_outputDevice)
		{
			ma_device_uninit(&_internals->outputDevice);
			SafeRelease(_outputDevice);
		}

		if(_inputDevice)
		{
			ma_device_uninit(&_internals->inputDevice);
			SafeRelease(_inputDevice);
		}

		ma_context_uninit(&_internals->context);

		delete _internals;
	}
	
	void ResonanceAudioSystemMiniAudio::AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, uint32 frameCount)
	{
		ResonanceAudioSystemMiniAudio *instance = static_cast<ResonanceAudioSystemMiniAudio*>(pDevice->pUserData);
		if(instance->_audioCallback)
		{
			instance->_audioCallback(pOutput, pInput, frameCount, 0);
		}
	}
	
	void ResonanceAudioSystemMiniAudio::SetOutputDevice(ResonanceAudioDevice *outputDevice)
	{
		RN_ASSERT(outputDevice || outputDevice->type == ResonanceAudioDevice::Type::Output, "Not an output device!");

		if(_outputDevice)
		{
			ma_device_uninit(&_internals->outputDevice);
		}
		SafeRelease(_outputDevice);
		_outputDevice = outputDevice;
		SafeRetain(_outputDevice);

		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.pDeviceID = static_cast<ma_device_id*>(outputDevice->Downcast<ResonanceAudioDeviceMiniAudio>()->deviceID);
		config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
		config.playback.channels = _channelCount;               // Set to 0 to use the device's native channel count.
		config.sampleRate = _sampleRate;           // Set to 0 to use the device's native sample rate.
		config.periodSizeInFrames = _frameSize;
		config.dataCallback = &AudioCallback;   // This function will be called when miniaudio needs more data.
		config.pUserData = this;   // Can be accessed from the device object (device.pUserData).

		if(ma_device_init(&_internals->context, &config, &_internals->outputDevice) != MA_SUCCESS)
		{
			return;  // Failed to initialize the device.
		}

		ma_device_start(&_internals->outputDevice);     // The device is sleeping by default so you'll need to start it manually.

		RNInfo("Using audio output device: " << _outputDevice->name);
	}

	void ResonanceAudioSystemMiniAudio::SetInputDevice(ResonanceAudioDevice *inputDevice)
	{
		RN_ASSERT(!inputDevice || inputDevice->type == ResonanceAudioDevice::Type::Input, "Not an input device!");

		if(_inputDevice)
		{
			ma_device_uninit(&_internals->inputDevice);
		}

		SafeRelease(_inputDevice);
		_inputDevice = inputDevice;
		SafeRetain(_inputDevice);

		ma_device_config config = ma_device_config_init(ma_device_type_capture);
		config.playback.pDeviceID = static_cast<ma_device_id*>(inputDevice->Downcast<ResonanceAudioDeviceMiniAudio>()->deviceID);
		config.playback.format = ma_format_f32;   // Set to ma_format_unknown to use the device's native format.
		config.playback.channels = _channelCount;               // Set to 0 to use the device's native channel count.
		config.sampleRate = _sampleRate;           // Set to 0 to use the device's native sample rate.
		config.dataCallback = &AudioCallback;   // This function will be called when miniaudio needs more data.
		config.pUserData = this;   // Can be accessed from the device object (device.pUserData).

		if(ma_device_init(&_internals->context, &config, &_internals->inputDevice) != MA_SUCCESS)
		{
			return;  // Failed to initialize the device.
		}

		ma_device_start(&_internals->inputDevice);     // The device is sleeping by default so you'll need to start it manually.
		
		RNInfo("Using audio input device: " << _inputDevice->name);
	}

	Array *ResonanceAudioSystemMiniAudio::GetDevices()
	{
		Array *deviceArray = new Array();

		ma_device_info* playbackInfos;
		ma_uint32 playbackCount;
		ma_device_info* captureInfos;
		ma_uint32 captureCount;
		if(ma_context_get_devices(&_internals->context, &playbackInfos, &playbackCount, &captureInfos, &captureCount) != MA_SUCCESS)
		{
			// Error.
			return deviceArray->Autorelease();
		}

		for(ma_uint32 i = 0; i < playbackCount; i += 1)
		{
			ma_device_id *deviceID = new ma_device_id;
			*deviceID = playbackInfos[i].id;
			ResonanceAudioDeviceMiniAudio *outputDevice = new ResonanceAudioDeviceMiniAudio(ResonanceAudioDevice::Type::Output, deviceID, playbackInfos[i].name, playbackInfos[i].isDefault || playbackCount == 1);
			deviceArray->AddObject(outputDevice->Autorelease());
		}

		for(ma_uint32 i = 0; i < captureCount; i += 1)
		{
			ma_device_id *deviceID = new ma_device_id;
			*deviceID = captureInfos[i].id;
			ResonanceAudioDeviceMiniAudio *inputDevice = new ResonanceAudioDeviceMiniAudio(ResonanceAudioDevice::Type::Input, deviceID, captureInfos[i].name, captureInfos[i].isDefault || captureCount == 1);
			deviceArray->AddObject(inputDevice->Autorelease());
		}
		
		return deviceArray->Autorelease();
	}

	ResonanceAudioDevice *ResonanceAudioSystemMiniAudio::GetDefaultOutputDevice()
	{
		RN::Array *devices = GetDevices();
		if(!devices) return nullptr;

		ResonanceAudioDevice *outputDevice = nullptr;
		devices->Enumerate<ResonanceAudioDevice>([&](ResonanceAudioDevice *device, size_t index, bool &stop) {
			if(device->type == ResonanceAudioDevice::Type::Output && device->isDefault)
			{
				outputDevice = device;
			}
		});

		return outputDevice; //Will be autoreleased as part of the devices array
	}

	ResonanceAudioDevice *ResonanceAudioSystemMiniAudio::GetDefaultInputDevice()
	{
		RN::Array *devices = GetDevices();
		if(!devices) return nullptr;

		ResonanceAudioDevice *inputDevice = nullptr;
		devices->Enumerate<ResonanceAudioDevice>([&](ResonanceAudioDevice *device, size_t index, bool &stop) {
			if(device->type == ResonanceAudioDevice::Type::Input && device->isDefault)
			{
				inputDevice = device;
			}
		});

		return inputDevice; //Will be autoreleased as part of the devices array
	}
}
