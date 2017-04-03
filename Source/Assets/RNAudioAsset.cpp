//
//  RNAudioAsset.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAudioAsset.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(AudioAsset, Asset)
	
	AudioAsset::AudioAsset()
	{
		
	}
	
	AudioAsset::~AudioAsset()
	{
		_data->Release();
	}
	
	void AudioAsset::SetRawAudioData(Data *data, int bitsPerSample, int sampleRate, int channels)
	{
		_data = data->Retain();
		_bitsPerSample = bitsPerSample;
		_sampleRate = sampleRate;
		_channels = channels;
	}

	AudioAsset *AudioAsset::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<AudioAsset>(name, settings);
	}
}
