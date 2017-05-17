//
//  RNSteamAudioWorld.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioWorld.h"
#include "soundio/soundio.h"
#include "phonon.h"

namespace RN
{
	RNDefineMeta(SteamAudioDevice, Object)
	RNDefineMeta(SteamAudioWorld, SceneAttachment)

	Array *SteamAudioWorld::_audioSources = new Array();
	float *SteamAudioWorld::_ambisonicsFrameData = nullptr;
	float *SteamAudioWorld::_outputFrameData = nullptr;
	uint32 SteamAudioWorld::_frameSize = 0;
	void *SteamAudioWorld::_ambisonicsBinauralEffect = nullptr;
	
	void SteamAudioWorld::WriteCallback(struct SoundIoOutStream *outstream, int sample_count_min, int sample_count_max)
	{
		if (_audioSources->GetCount() == 0)
			return;

		const struct SoundIoChannelLayout *layout = &outstream->layout;
		float seconds_per_sample = 1.0f / static_cast<float>(outstream->sample_rate);
		struct SoundIoChannelArea *areas;
		int samples_left = std::max(static_cast<int>(_frameSize), sample_count_min);

		while(samples_left > 0)
		{
			int sampleCount = samples_left;

			soundio_outstream_begin_write(outstream, &areas, &sampleCount);
			if(!sampleCount)
				break;

			IPLAudioFormat inputFormat;
			inputFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
			inputFormat.numSpeakers = 4;	//TODO: Experiment with the value, it apparently HAS to be set to something...
			inputFormat.ambisonicsOrder = 2;	//TODO: Experiment with the value
			inputFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
			inputFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
			inputFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;
			IPLAudioBuffer inputBuffer[2];
			inputBuffer[0].format = inputFormat;
			inputBuffer[1].format = inputFormat;
			inputBuffer[0].numSamples = sampleCount;
			inputBuffer[1].numSamples = sampleCount;
			inputBuffer[1].interleavedBuffer = _ambisonicsFrameData;

			float secondsPerFrame = seconds_per_sample * sampleCount;
			_audioSources->Enumerate<SteamAudioSource>([&](SteamAudioSource *source, size_t index, bool &stop) {
				float *outData = nullptr;
				source->Update(secondsPerFrame, sampleCount, &outData);

				inputBuffer[0].interleavedBuffer = outData;
				iplMixAudioBuffers(2, inputBuffer, inputBuffer[1]);
			});

			IPLAudioFormat outputFormat;
			outputFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
			outputFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
			outputFormat.numSpeakers = 2;
			outputFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

			IPLAudioBuffer outputBuffer;
			outputBuffer.format = outputFormat;
			outputBuffer.numSamples = sampleCount;
			outputBuffer.interleavedBuffer = _outputFrameData;
			iplApplyAmbisonicsBinauralEffect(_ambisonicsBinauralEffect, inputBuffer[1], outputBuffer);

			for(int sample = 0; sample < sampleCount; sample++)
			{
				for(int channel = 0; channel < layout->channel_count; channel++)
				{
					float *ptr = reinterpret_cast<float*>(areas[channel].ptr + areas[channel].step * sample);
					*ptr = _outputFrameData[sample * layout->channel_count + channel];
				}
			}

			soundio_outstream_end_write(outstream);

			samples_left -= sampleCount;
		}
	}

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	SteamAudioWorld::SteamAudioWorld(SteamAudioDevice *outputDevice, uint32 sampleRate, uint32 frameSize) :
		_audioListener(nullptr)
	{
		RN_ASSERT(!outputDevice || outputDevice->type == SteamAudioDevice::Type::Output, "outputDevice has to be an output device!");

		//Initialize libsoundio
		_soundio = soundio_create();
		soundio_connect(_soundio);
		soundio_flush_events(_soundio);

		int deviceIndex = soundio_default_output_device_index(_soundio);
		if (outputDevice)
			deviceIndex = outputDevice->index;
		if(deviceIndex < 0)
		{
			RNDebug("No audio output device found.");
			return;
		}

		_device = soundio_get_output_device(_soundio, deviceIndex);
		if(!_device)
		{
			RNDebug("Failed opening audio device.");
			return;
		}

		RNInfo("Using audio device: " << _device->name);

		_outstream = soundio_outstream_create(_device);
		_outstream->format = SoundIoFormatFloat32NE;
		_outstream->sample_rate = sampleRate;
		_outstream->write_callback = WriteCallback;

		int err;
		if((err = soundio_outstream_open(_outstream)))
		{
			RNDebug("Failed opening audio device with error:" << soundio_strerror(err));
			return;
		}

		if(_outstream->layout_error)
			RNDebug("Unable to set channel layout with error:" << soundio_strerror(_outstream->layout_error));

		if((err = soundio_outstream_start(_outstream)))
		{
			RNDebug("Failed opening audio device with error:" << soundio_strerror(err));
			return;
		}

		//Initialize Steam Audio
		IPLContext context{nullptr, nullptr, nullptr};
		const int32 samplingrate = sampleRate;
		const int32 framesize = frameSize;
		IPLRenderingSettings settings{samplingrate, framesize};
		IPLHrtfParams hrtfParams{IPL_HRTFDATABASETYPE_DEFAULT, nullptr, 0, nullptr, nullptr};
		iplCreateBinauralRenderer(context, settings, hrtfParams, &_binauralRenderer); //TODO: HRTF is only a good idea with headphones, add option to disable

		IPLAudioFormat inputFormat;
		inputFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		inputFormat.numSpeakers = 4;	//TODO: Experiment with the value, it apparently HAS to be set to something...
		inputFormat.ambisonicsOrder = 2;	//TODO: Experiment with the value
		inputFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
		inputFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		inputFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		IPLAudioFormat outputFormat;
		outputFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		outputFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		outputFormat.numSpeakers = 2;
		outputFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		iplCreateAmbisonicsBinauralEffect(_binauralRenderer, inputFormat, outputFormat, &_ambisonicsBinauralEffect);

		_frameSize = frameSize;
		_ambisonicsFrameData = new float[_frameSize * 15];
		_outputFrameData = new float[_frameSize * 2];
	}
		
	SteamAudioWorld::~SteamAudioWorld()
	{
		soundio_outstream_destroy(_outstream);
		soundio_device_unref(_device);
		soundio_destroy(_soundio);

		iplDestroyAmbisonicsBinauralEffect(&_ambisonicsBinauralEffect);
		iplDestroyBinauralRenderer(&_binauralRenderer);

		SafeRelease(_audioSources);

		if(_frameSize > 0)
		{
			delete[] _ambisonicsFrameData;
			delete[] _outputFrameData;
			_ambisonicsFrameData = nullptr;
			_outputFrameData = nullptr;
			_frameSize = 0;
		}
	}

	Array *SteamAudioWorld::GetDevices()
	{
		//Initialize libsoundio
		SoundIo *soundio = soundio_create();
		SoundIoDevice *device;
		soundio_connect(soundio);
		soundio_flush_events(soundio);

		Array *deviceArray = new Array();

		//Get all output devices, put the default one first
		int outputDeviceCount = soundio_output_device_count(soundio);
		for(int i = 0; i < outputDeviceCount; i++)
		{
			device = soundio_get_output_device(soundio, i);
			if(device)
			{
				SteamAudioDevice *saDevice = new SteamAudioDevice(SteamAudioDevice::Type::Output, i, device->name, device->id, false);
				deviceArray->AddObject(saDevice->Autorelease());
				soundio_device_unref(device);
			}
		}

		//Get all input devices, put the default one first
		int inputDeviceCount = soundio_input_device_count(soundio);
		for(int i = 0; i < inputDeviceCount; i++)
		{
			device = soundio_get_input_device(soundio, i);
			if(device)
			{
				SteamAudioDevice *saDevice = new SteamAudioDevice(SteamAudioDevice::Type::Input, i, device->name, device->id, false);
				deviceArray->AddObject(saDevice->Autorelease());
				soundio_device_unref(device);
			}
		}

		soundio_destroy(soundio);

		return deviceArray->Autorelease();
	}
		
	void SteamAudioWorld::Update(float delta)
	{
//		soundio_flush_events(_soundio);	//TODO: Call this and handle changing devices
	}
		
	void SteamAudioWorld::SetListener(SteamAudioListener *attachment)
	{
		if(_audioListener)
			_audioListener->RemoveFromWorld();
			
		_audioListener = attachment;
			
		if(_audioListener)
			_audioListener->InsertIntoWorld(this);
	}
		
	void SteamAudioWorld::PlaySound(AudioAsset *resource)
	{
		if(_audioListener)
		{
			SteamAudioSource *source = new SteamAudioSource(resource, this);
			source->SetSelfdestruct(true);
			source->Play();
			_audioSources->AddObject(source->Autorelease());
		}
	}
}
