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
	RNDefineMeta(SteamAudioWorld, SceneAttachment)

	Array *SteamAudioWorld::_audioSources = new Array();
	
	void SteamAudioWorld::WriteCallback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
	{
		const struct SoundIoChannelLayout *layout = &outstream->layout;
		float float_sample_rate = outstream->sample_rate;
		float seconds_per_frame = 1.0f / float_sample_rate;
		struct SoundIoChannelArea *areas;
		int frames_left = std::max(1024, frame_count_min);

		while(frames_left > 0)
		{
			int frame_count = frames_left;

			soundio_outstream_begin_write(outstream, &areas, &frame_count);

			if(!frame_count)
				break;

			for(int frame = 0; frame < frame_count; frame += 1)
			{
				float sample = 0.0f;
				_audioSources->Enumerate<SteamAudioSource>([&](SteamAudioSource *source, size_t index, bool &stop){
					if(source)
					{
						sample += source->GetSample(0);
						source->Update(seconds_per_frame);
					}
				});

				for(int channel = 0; channel < layout->channel_count; channel += 1)
				{
					float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
					*ptr = sample;
				}
			}

			soundio_outstream_end_write(outstream);

			frames_left -= frame_count;
		}
	}

		
	SteamAudioWorld::SteamAudioWorld(String *deviceName) :
		_audioListener(nullptr)
	{
		//Initialize libsoundio
		_soundio = soundio_create();
		soundio_connect(_soundio);
		soundio_flush_events(_soundio);

		int default_out_device_index = soundio_default_output_device_index(_soundio);
		if(default_out_device_index < 0)
		{
			RNDebug("No audio output device found.");
			return;
		}

		_device = soundio_get_output_device(_soundio, default_out_device_index);
		if(!_device)
		{
			RNDebug("Failed opening audio device.");
			return;
		}

		RNInfo("Using audio device: " << _device->name);

		_outstream = soundio_outstream_create(_device);
		_outstream->format = SoundIoFormatFloat32NE;
		_outstream->sample_rate = 44100;
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
/*		IPLContext context{nullptr, nullptr, nullptr};
		const int32 samplingrate = 44100;
		const int32 framesize = 1024;
		IPLRenderingSettings settings{samplingrate, framesize};
		IPLhandle renderer{nullptr};
		IPLHrtfParams hrtfParams{IPL_HRTFDATABASETYPE_DEFAULT, nullptr, 0, nullptr, nullptr};
		iplCreateBinauralRenderer(context, settings, hrtfParams, &renderer); //TODO: HRTF is only a good idea with headphones, add option to disable

		IPLAudioFormat mono;
		mono.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		mono.channelLayout = IPL_CHANNELLAYOUT_MONO;
		mono.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		IPLAudioFormat stereo;
		stereo.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		stereo.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		stereo.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		IPLhandle effect{nullptr};
		iplCreateBinauralEffect(renderer, mono, stereo, &effect);

		IPLAudioBuffer inbuffer{mono, framesize, inputaudio.data()};
		std::vector<float> outputaudioframe(2 * framesize);
		IPLAudioBuffer outbuffer{stereo, framesize, outputaudioframe.data()};

		for(uint32 i = 0; i < numframes; ++i)
		{
			iplApplyBinauralEffect(effect, inbuffer, IPLVector3{ 1.0f, 1.0f, 1.0f }, IPL_HRTFINTERPOLATION_NEAREST, outbuffer);
			std::copy(std::begin(outputaudioframe), std::end(outputaudioframe), std::back_inserter(outputaudio));
			inbuffer.interleavedBuffer += framesize;
		}*/
	}
		
	SteamAudioWorld::~SteamAudioWorld()
	{
		soundio_outstream_destroy(_outstream);
		soundio_device_unref(_device);
		soundio_destroy(_soundio);

/*		iplDestroyBinauralEffect(&effect);
		iplDestroyBinauralRenderer(&renderer);*/

		SafeRelease(_audioSources);
	}

	Array *SteamAudioWorld::GetDeviceNames()
	{
/*		const char *bytes = static_cast<const char*>(alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER));
		Array *devices = new Array();
		String *deviceString = String::WithString(bytes, true);
		while(deviceString->GetLength() > 0)
		{
			devices->AddObject(deviceString);
			bytes += deviceString->GetLength() + 1;
			deviceString = String::WithString(bytes, true);
		}
		
		return devices;*/

		return nullptr;
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
		
	SteamAudioSource *SteamAudioWorld::PlaySound(AudioAsset *resource)
	{
		if(_audioListener)
		{
			SteamAudioSource *source = new SteamAudioSource(resource);
			source->SetSelfdestruct(true);
			source->Play();
			_audioSources->AddObject(source);
			return source;
		}
		return nullptr;
	}
}
