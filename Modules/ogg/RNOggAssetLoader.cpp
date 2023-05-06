//
//  RNOggAssetLoader.cpp
//  Rayne-Ogg
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOggAssetLoader.h"

namespace RN
{
	namespace vorbis
	{
		//#define STB_VORBIS_NO_STDIO
		#include "stb_vorbis.h"
	}
	
	RNDefineMeta(OggAssetLoader, AssetLoader)
	RNDefineMeta(OggAudioDecoder, AudioDecoder)

	static OggAssetLoader *__assetLoader;

	void OggAssetLoader::InitialWakeUp(MetaClass *meta)
	{
		if(meta == OggAssetLoader::GetMetaClass())
		{
			Config config({ AudioAsset::GetMetaClass() });
			config.SetExtensions(Set::WithObjects({ RNCSTR("ogg") }));
			config.supportsBackgroundLoading = true;

			__assetLoader = new OggAssetLoader(config);

			AssetManager *manager = AssetManager::GetSharedInstance();
			manager->RegisterAssetLoader(__assetLoader);
		}
	}

	OggAssetLoader::OggAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	Asset *OggAssetLoader::Load(File *file, const LoadOptions &options)
	{
		AudioAsset *audio = nullptr;
		
		Number *wantsStreaming = options.settings->GetValueForKey<Number>("wantsStreaming");
		if(wantsStreaming && wantsStreaming->GetBoolValue())
		{
			OggAudioDecoder *audioDecoder = new OggAudioDecoder(file);
			audio = new RN::AudioAsset(audioDecoder, /*5 * audioDecoder->_frameSize * audioDecoder->_channelCount*/ 3840 * 5 * 2, audioDecoder->_bytesPerSample, audioDecoder->_sampleRate, audioDecoder->_channelCount);
		}
		else
		{
			short *audioData = nullptr;
			int channels = 0;
			int sample_rate = 0;
			
			Data *fileData = file->ReadData(file->GetSize());
			int samples = vorbis::stb_vorbis_decode_memory(static_cast<uint8*>(fileData->GetBytes()), static_cast<unsigned int>(fileData->GetLength()), &channels, &sample_rate, &audioData);

			audio = new RN::AudioAsset();
			Data *data = new Data(reinterpret_cast<uint8*>(audioData), samples * channels * 2);
			audio->SetRawAudioData(data->Autorelease(), channels * 2, sample_rate, channels);
			free(audioData);
		}

		return audio;
	}

	
	OggAudioDecoder::OggAudioDecoder(File *file) : AudioDecoder(4096), _file(file->Retain())
	{
		int error = 0;
		_vorbis = vorbis::stb_vorbis_open_file(file->CreateFilePtr(), 0, &error, nullptr);
		vorbis::stb_vorbis_info vorbisInfo = vorbis::stb_vorbis_get_info(_vorbis);
		_frameSize = 4096 * 2; //Maximum ogg vorbis supported frame size, the one from info seems incorrect//vorbisInfo.max_frame_size * 2;
		_channelCount = vorbisInfo.channels;
		_bytesPerSample = 2 * _channelCount;
		_sampleRate = vorbisInfo.sample_rate;
		
		uint32 frameSize = _frameSize / 2 * _channelCount;
		_buffer = new short[frameSize];
	}

	uint32 OggAudioDecoder::DecodeFrameToAudioAsset(AudioAsset *audioAsset)
	{
		uint32 frameSize = _frameSize / 2 * _channelCount;
		uint32 actualSamples = vorbis::stb_vorbis_get_frame_short_interleaved(_vorbis, _channelCount, _buffer, frameSize);
		audioAsset->PushData(_buffer, actualSamples * _channelCount * 2);
		
		return actualSamples * _channelCount * 2;
	}

	void OggAudioDecoder::Seek(float time)
	{
		vorbis::stb_vorbis_seek(_vorbis, time/_sampleRate);
	}
}
