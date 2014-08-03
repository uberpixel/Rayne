//
//  RNAudioResource.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAudioResource.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	RNDefineMeta(AudioResource, Asset)
	
	AudioResource::AudioResource()
	{
		
	}
	
	AudioResource::~AudioResource()
	{
		_data->Release();
	}
	
	void AudioResource::SetRawAudioData(Data *data, int bitsPerSample, int sampleRate, int channels)
	{
		_data = data->Retain();
		_bitsPerSample = bitsPerSample;
		_sampleRate = sampleRate;
		_channels = channels;
	}
	
	AudioResource *AudioResource::WithFile(const std::string& path)
	{
		Dictionary *finalsettings = new Dictionary();
		finalsettings->Autorelease();
		
		AudioResource *audio = RN::ResourceCoordinator::GetSharedInstance()->GetResourceWithName<AudioResource>(RNSTR(path.c_str()), finalsettings);
		return audio;
	}
}
