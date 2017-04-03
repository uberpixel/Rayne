//
//  RNOggAssetLoader.cpp
//  Rayne-Ogg
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOggAssetLoader.h"
#include "stb_vorbis.h"

namespace RN
{
	RNDefineMeta(OggAssetLoader, AssetLoader)

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
		short *audioData = nullptr;
		int channels = 0;
		int sample_rate = 0;
		RN::Data *fileData = file->ReadData(file->GetSize());
		int samples = stb_vorbis_decode_memory(static_cast<uint8*>(fileData->GetBytes()), static_cast<unsigned int>(fileData->GetLength()), &channels, &sample_rate, &audioData);

		RN::AudioAsset *audio = new RN::AudioAsset();
		RN::Data *data = new RN::Data(reinterpret_cast<uint8*>(audioData), samples * channels * 2);
		audio->SetRawAudioData(data, 16, sample_rate, channels);
		free(audioData);

		return audio;
	}
}
