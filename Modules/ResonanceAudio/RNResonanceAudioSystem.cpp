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

	ResonanceAudioSystem::ResonanceAudioSystem(uint32 sampleRate, uint32 frameSize, uint8 channelCount) :
		_frameSize(frameSize), _sampleRate(sampleRate), _channelCount(channelCount)
	{}

	ResonanceAudioSystem::~ResonanceAudioSystem()
	{
		SetOutputSamplesCallback(nullptr);
		RemoveAllListenerContexts();
	}

	void ResonanceAudioSystem::WaitForOutputCallback()
	{
		// Publication and entry use the same sequentially consistent order. A
		// later entrant can only see the new pointers. Wait for at most the
		// render already in progress, even if the device keeps calling back.
		const uint32 sequence = _outputCallbackSequence.load();
		while((sequence & 1U) && _outputCallbackSequence.load() == sequence)
		{
			std::this_thread::yield();
		}
	}

	void ResonanceAudioSystem::SetOutputSamplesCallback(OutputSamplesCallback callback)
	{
		auto *replacement = callback ? new OutputSamplesCallback(std::move(callback)) : nullptr;
		std::lock_guard<std::mutex> lock(_outputSamplesLock);
		auto *previous = _outputSamplesCallback.exchange(replacement);
		if(previous) WaitForOutputCallback();
		delete previous;
	}

	void ResonanceAudioSystem::SetRecordingContext(ResonanceAudioListenerContext *context)
	{
		if(_recordingContext.exchange(context)) WaitForOutputCallback();
	}

	void ResonanceAudioSystem::RenderAudio(void *outputBuffer, const void *inputBuffer, uint32 frameCount, uint32 sampleRate, uint32 channelCount, uint32 status)
	{
		if(inputBuffer)
		{
			if(auto *primary = GetListenerContext())
			{
				primary->CaptureInput(static_cast<const float *>(inputBuffer), sampleRate, channelCount, frameCount);
			}
		}
		if(!outputBuffer) return;

		const uint64 generation = _outputGeneration.load(std::memory_order_relaxed);
		const uint64 firstSampleFrame = _nextOutputSampleFrame;
		_nextOutputSampleFrame += frameCount;

		const bool hasTap = _recordingContext.load() || _outputSamplesCallback.load();
		if(hasTap) _outputCallbackSequence.fetch_add(1);
		ResonanceAudioListenerContext *recording = hasTap ? _recordingContext.load() : nullptr;
		OutputSamplesCallback *tap = hasTap ? _outputSamplesCallback.load() : nullptr;

		auto *output = static_cast<float *>(outputBuffer);
		if(auto *primary = GetListenerContext())
		{
			primary->_world->RenderOutput(*primary, output, frameCount, sampleRate, channelCount, recording);
		}
		else
		{
			memset(output, 0, frameCount * channelCount * sizeof(float));
		}

		if(recording)
		{
			recording->_recordingCallback({recording->_recordingOutput, frameCount, sampleRate, 2, firstSampleFrame, generation});
		}
		if(tap) (*tap)(output, frameCount, sampleRate, channelCount);
		if(hasTap) _outputCallbackSequence.fetch_add(1);
	}

	ResonanceAudioListenerContext *ResonanceAudioSystem::CreateListenerContext(ResonanceAudioWorld *world)
	{
		RN_ASSERT(world && !GetListenerContext(), "Expected an owning world with no listener context");
		auto *context = new ResonanceAudioListenerContext(world, _channelCount, _frameSize, _sampleRate);
		_primaryContext.store(context, std::memory_order_release);
		return context;
	}

	void ResonanceAudioSystem::RemoveAllListenerContexts()
	{
		// The world stops both devices before detaching the primary context.
		auto *context = _primaryContext.exchange(nullptr, std::memory_order_acq_rel);
		if(!context) return;
		context->_world = nullptr;
		context->SetInputSamplesCallback(nullptr);
		context->SetListener(nullptr);
		context->Release();
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
		SetOutputDevice(nullptr);
		SetInputDevice(nullptr);

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
		RN_ASSERT(!outputDevice || outputDevice->type == ResonanceAudioDevice::Type::Output, "Not an output device!");

		if(_outputDevice)
		{
			ma_device_uninit(&_internals->outputDevice);
		}
		SafeRelease(_outputDevice);
		_outputDevice = outputDevice;
		SafeRetain(_outputDevice);
		if(!_outputDevice) return;

		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.pDeviceID = static_cast<ma_device_id *>(outputDevice->Downcast<ResonanceAudioDeviceMiniAudio>()->deviceID);
		config.playback.format = ma_format_f32; // Set to ma_format_unknown to use the device's native format.
		config.playback.channels = _channelCount; // Set to 0 to use the device's native channel count.
		config.sampleRate = _sampleRate; // Set to 0 to use the device's native sample rate.
		// Resonance consumes one fixed DSP block. Miniaudio slices or gathers
		// native device buffers to this period before calling AudioCallback.
		config.periodSizeInFrames = _frameSize;
		config.noFixedSizedCallback = MA_FALSE;
		config.notificationCallback = [](const ma_device_notification *notification) {
			switch(notification->type)
			{
				case ma_device_notification_type_rerouted:
				case ma_device_notification_type_interruption_ended:
					static_cast<ResonanceAudioSystemMiniAudio *>(notification->pDevice->pUserData)->AdvanceOutputGeneration();
					break;
				default:
					break;
			}
		};
		config.dataCallback = &AudioCallback; // This function will be called when miniaudio needs more data.
		config.pUserData = this; // Can be accessed from the device object (device.pUserData).

		if(ma_device_init(&_internals->context, &config, &_internals->outputDevice) != MA_SUCCESS)
		{
			SafeRelease(_outputDevice);
			return; // Failed to initialize the device.
		}

		AdvanceOutputGeneration(); // Some backends request output while starting.
		if(ma_device_start(&_internals->outputDevice) != MA_SUCCESS)
		{
			ma_device_uninit(&_internals->outputDevice);
			SafeRelease(_outputDevice);
			return;
		}

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

#if RN_PLATFORM_ANDROID
		config.aaudio.inputPreset = ma_aaudio_input_preset_voice_communication;
		config.opensl.recordingPreset = ma_opensl_recording_preset_voice_communication;
#endif

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
