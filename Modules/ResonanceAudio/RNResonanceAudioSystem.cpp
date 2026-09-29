//
//  RNResonanceAudioSystem.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioSystem.h"
#include "RNResonanceAudioInternals.h"
#include "RNResonanceAudioListenerContext.h"
#include "RNResonanceAudioWorld.h"

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
	ResonanceAudioSystem::ResonanceAudioSystem(uint32 sampleRate, uint32 frameSize, uint8 channelCount) :
		_frameSize(frameSize), _sampleRate(sampleRate), _channelCount(channelCount)
	{
		for(int i = 0; i < 3; i++)
		{
			_listenerContextSnapshots[i] = new Array();
		}
	}

	ResonanceAudioSystem::~ResonanceAudioSystem()
	{
		RemoveAllListenerContexts();
		for(int i = 0; i < 3; i++)
		{
			SafeRelease(_listenerContextSnapshots[i]);
		}
	}

	void ResonanceAudioSystem::RenderAudio(void *outputBuffer, const void *inputBuffer, uint32 frameCount, uint32 sampleRate, uint32 channelCount, uint32 status)
	{
		const uint32 snapshotIndex = _listenerContextSnapshotIndex.load(std::memory_order_acquire) % 3;
		_listenerContextSnapshotInUseIndex.store(snapshotIndex, std::memory_order_relaxed);
		Array *snapshot = _listenerContextSnapshots[snapshotIndex];

		if(snapshot && snapshot->GetCount() > 0)
		{
			ResonanceAudioListenerContext *listenerContext = snapshot->GetObjectAtIndex<ResonanceAudioListenerContext>(0);
			if(listenerContext)
			{
				listenerContext->RenderAudio(outputBuffer, inputBuffer, sampleRate, channelCount, frameCount, status);
				return;
			}
		}
		
		if(outputBuffer)
		{
			memset(outputBuffer, 0, frameCount * channelCount * sizeof(float));
		}
	}

	ResonanceAudioListenerContext *ResonanceAudioSystem::CreateListenerContext()
	{
		ResonanceAudioWorld *world = _owningWorld;
		RN_ASSERT(world, "Owning world must be set before creating listener contexts!");

		ResonanceAudioListenerContext *context = new ResonanceAudioListenerContext(world, _channelCount, _frameSize, _sampleRate);
		context->Retain();
		_listenerContexts.push_back(context);
		PublishListenerContextsSnapshot();

		return context;
	}

	void ResonanceAudioSystem::RemoveAllListenerContexts()
	{
		for(ResonanceAudioListenerContext *context : _listenerContexts)
		{
			if(context) context->Release();
		}
		_listenerContexts.clear();
		PublishListenerContextsSnapshot();
	}

	ResonanceAudioListenerContext *ResonanceAudioSystem::GetListenerContext() const
	{
		if(_listenerContexts.empty()) return nullptr;
		return _listenerContexts.front();
	}

	void ResonanceAudioSystem::PublishListenerContextsSnapshot()
	{
		const uint32 inUse = _listenerContextSnapshotInUseIndex.load(std::memory_order_relaxed) % 3;
		const uint32 published = _listenerContextSnapshotIndex.load(std::memory_order_relaxed) % 3;
		uint32 writeIndex = (published + 1) % 3;
		if(writeIndex == inUse) writeIndex = (writeIndex + 1) % 3;

		Array *fresh = _listenerContextSnapshots[writeIndex];
		fresh->RemoveAllObjects();
		for(ResonanceAudioListenerContext *context : _listenerContexts)
		{
			if(!context) continue;
			fresh->AddObject(context); // retain for audio thread
		}
		_listenerContextSnapshotIndex.store(writeIndex, std::memory_order_release);
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
		ma_device_id *typedDeviceID = static_cast<ma_device_id *>(deviceID);
		if(typedDeviceID) delete typedDeviceID;
	}

	ResonanceAudioSystemMiniAudio::ResonanceAudioSystemMiniAudio(uint32 sampleRate, uint32 frameSize, uint8 channelCount) :
		ResonanceAudioSystem(sampleRate, frameSize, channelCount), _internals(new ResonanceAudioSystemMiniAudioInternals()), _outputDevice(nullptr), _inputDevice(nullptr)
	{
		if(ma_context_init(NULL, 0, NULL, &_internals->context) != MA_SUCCESS)
		{
			// Error.
		}
#if RN_PLATFORM_ANDROID && defined(MA_SUPPORT_AAUDIO)
		else if(_internals->context.backend == ma_backend_aaudio)
		{
			// Miniaudio lacks these attributes. Hook builder setup, including route changes.
			static MA_PFN_AAudioStreamBuilder_setDirection setDirection;
			static void (*setSpatialized)(ma_AAudioStreamBuilder *, bool);
			static void (*setBehavior)(ma_AAudioStreamBuilder *, int32_t);
			ma_context &context = _internals->context;
			setDirection = reinterpret_cast<decltype(setDirection)>(context.aaudio.AAudioStreamBuilder_setDirection);
			setSpatialized = reinterpret_cast<decltype(setSpatialized)>(ma_dlsym(ma_context_get_log(&context), context.aaudio.hAAudio, "AAudioStreamBuilder_setIsContentSpatialized"));
			setBehavior = reinterpret_cast<decltype(setBehavior)>(ma_dlsym(ma_context_get_log(&context), context.aaudio.hAAudio, "AAudioStreamBuilder_setSpatializationBehavior"));
			context.aaudio.AAudioStreamBuilder_setDirection = reinterpret_cast<ma_proc>(+[](ma_AAudioStreamBuilder *builder, ma_aaudio_direction_t direction) {
				setDirection(builder, direction);
				if(direction != MA_AAUDIO_DIRECTION_OUTPUT) return;
				if(setSpatialized) setSpatialized(builder, true);
				if(setBehavior) setBehavior(builder, 2); // AAUDIO_SPATIALIZATION_BEHAVIOR_NEVER, API 32.
			});
		}
#endif
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

	void ResonanceAudioSystemMiniAudio::AudioCallback(ma_device *pDevice, void *pOutput, const void *pInput, uint32 frameCount)
	{
		ResonanceAudioSystemMiniAudio *instance = static_cast<ResonanceAudioSystemMiniAudio *>(pDevice->pUserData);
		instance->RenderAudio(pOutput, pInput, frameCount, instance->_sampleRate, instance->_channelCount, 0);
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
		config.playback.pDeviceID = static_cast<ma_device_id *>(outputDevice->Downcast<ResonanceAudioDeviceMiniAudio>()->deviceID);
		config.playback.format = ma_format_f32; // Set to ma_format_unknown to use the device's native format.
		config.playback.channels = _channelCount; // Set to 0 to use the device's native channel count.
		config.sampleRate = _sampleRate; // Set to 0 to use the device's native sample rate.
		config.periodSizeInFrames = _frameSize;
		config.dataCallback = &AudioCallback; // This function will be called when miniaudio needs more data.
		config.pUserData = this; // Can be accessed from the device object (device.pUserData).

		if(ma_device_init(&_internals->context, &config, &_internals->outputDevice) != MA_SUCCESS)
		{
			return; // Failed to initialize the device.
		}

		ma_device_start(&_internals->outputDevice); // The device is sleeping by default so you'll need to start it manually.

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
		if(!_inputDevice) return;

		ma_device_config config = ma_device_config_init(ma_device_type_capture);
		config.capture.pDeviceID = static_cast<ma_device_id *>(inputDevice->Downcast<ResonanceAudioDeviceMiniAudio>()->deviceID);
		config.capture.format = ma_format_f32; // Set to ma_format_unknown to use the device's native format.
		config.capture.channels = _channelCount; // Set to 0 to use the device's native channel count.
		config.sampleRate = _sampleRate; // Set to 0 to use the device's native sample rate.
		config.dataCallback = &AudioCallback; // This function will be called when miniaudio needs more data.
		config.pUserData = this; // Can be accessed from the device object (device.pUserData).

		if(ma_device_init(&_internals->context, &config, &_internals->inputDevice) != MA_SUCCESS)
		{
			SafeRelease(_inputDevice);
			return; // Failed to initialize the device.
		}

		if(ma_device_start(&_internals->inputDevice) != MA_SUCCESS)
		{
			ma_device_uninit(&_internals->inputDevice);
			SafeRelease(_inputDevice);
			return;
		}

		RNInfo("Using audio input device: " << _inputDevice->name);
	}

	Array *ResonanceAudioSystemMiniAudio::GetDevices()
	{
		Array *deviceArray = new Array();

		ma_device_info *playbackInfos;
		ma_uint32 playbackCount;
		ma_device_info *captureInfos;
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
} // namespace RN
