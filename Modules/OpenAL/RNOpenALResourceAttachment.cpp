//
//  RNOpenALResourceAttachment.cpp
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenALResourceAttachment.h"

#include <AL/al.h>
#include <AL/alc.h>

static const char *kResourceAttachmentKey = "kResourceAttachmentKey";

namespace RN
{
	OpenALResourceAttachment::OpenALResourceAttachment(RN::AudioAsset *resource)
	{
		alGenBuffers(1, &_bufferID);
		alBufferData(_bufferID, GetALFormat(resource->GetChannels(), resource->GetBytesPerSample()*8), resource->GetData()->GetBytes(), static_cast<int>(resource->GetData()->GetLength()), resource->GetSampleRate());
	}
		
	OpenALResourceAttachment::~OpenALResourceAttachment()
	{
		alDeleteBuffers(1, &_bufferID);
	}
		
	int OpenALResourceAttachment::GetALFormat(short channels, short bitsPerSample)
	{
		bool stereo = (channels > 1);
			
		switch(bitsPerSample)
		{
			case 16:
				if (stereo)
					return AL_FORMAT_STEREO16;
				else
					return AL_FORMAT_MONO16;
			case 8:
				if (stereo)
					return AL_FORMAT_STEREO8;
				else
					return AL_FORMAT_MONO8;
			default:
				return -1;
		}
	}
		
	OpenALResourceAttachment *OpenALResourceAttachment::GetAttachmentForResource(RN::AudioAsset *resource)
	{
		RN::Object *object = resource->GetAssociatedObject(kResourceAttachmentKey);
		if(object)
		{
			OpenALResourceAttachment *resourceAttachment = object->Downcast<OpenALResourceAttachment>();
			RN_ASSERT(resourceAttachment, "Audio Resource attachment must be of Type AudioResourceAttachment");
			return resourceAttachment;
		}
			
		OpenALResourceAttachment *resourceAttachment = new OpenALResourceAttachment(resource);
		resource->SetAssociatedObject(kResourceAttachmentKey, resourceAttachment, RN::Object::MemoryPolicy::Retain);
		resourceAttachment->Release();
			
		return resourceAttachment;
	}
}
