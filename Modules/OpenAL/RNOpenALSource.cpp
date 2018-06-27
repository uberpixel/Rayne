//
//  RNOpenALSource.cpp
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenALSource.h"
#include "RNOpenALWorld.h"
#include "RNOpenALResourceAttachment.h"

#include "AL/al.h"
#include "AL/alc.h"

namespace RN
{
	RNDefineMeta(OpenALSource, SceneNode)

		OpenALSource::OpenALSource(AudioAsset *asset) :
		_asset(asset),
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_hasEnded(false)
	{
		SafeRetain(_asset);
		_oldPosition = GetWorldPosition();
			
		alGenSources(1, &_source);
		alSourcef(_source, AL_PITCH, 1);
		alSourcef(_source, AL_GAIN, 1);
		alSourcei(_source, AL_LOOPING, AL_FALSE);

		SetAudioAsset(asset);
	}
		
	OpenALSource::~OpenALSource()
	{
		alDeleteSources(1, &_source);
		SafeRelease(_asset);
	}

	void OpenALSource::SetAudioAsset(AudioAsset *asset)
	{
		alSourcei(_source, AL_BUFFER, 0);

		SafeRelease(_asset);
		_asset = asset;
		SafeRetain(_asset);

		if(_asset)
		{
			OpenALResourceAttachment *attachment = OpenALResourceAttachment::GetAttachmentForResource(asset);
			alSourcei(_source, AL_BUFFER, attachment->GetBufferID());
		}
	}
		
	void OpenALSource::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
		alSourcei(_source, AL_LOOPING, repeat?AL_TRUE: AL_FALSE);
	}
		
	void OpenALSource::SetPitch(float pitch)
	{
		alSourcef(_source, AL_PITCH, pitch);
	}
		
	void OpenALSource::SetGain(float gain)
	{
		alSourcef(_source, AL_GAIN, gain);
	}
		
	void OpenALSource::SetRange(float min, float max)
	{
		alSourcei(_source, AL_REFERENCE_DISTANCE, min);
		alSourcei(_source, AL_MAX_DISTANCE, max);
	}
		
	void OpenALSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing = selfdestruct;
	}
		
	void OpenALSource::Play()
	{
		Update(0.0f);

		alSourcePlay(_source);
		_isPlaying = true;
		_hasEnded = false;
	}

	void OpenALSource::Stop()
	{
		alSourceStop(_source);
        _isPlaying = false;
	}

    void OpenALSource::Seek(float time)
    {
		alSourcef(_source, AL_SEC_OFFSET, time);
    }
		
	void OpenALSource::Update(float delta)
	{
		Vector3 position = GetWorldPosition();
		alSourcefv(_source, AL_POSITION, &position.x);

		Vector3 velocity = position - _oldPosition;
		_oldPosition = position;

		if(delta == 0.0f)
			return;

		velocity /= delta;
		alSourcefv(_source, AL_VELOCITY, &velocity.x);
			
		ALenum sourceState;
		alGetSourcei(_source, AL_SOURCE_STATE, &sourceState);
		if(sourceState == AL_STOPPED)
		{
			_isPlaying = false;
			_hasEnded = true;
			if(_isSelfdestructing)
			{
				if(GetScene())
					GetScene()->RemoveNode(this);
			}
		}
	}
}
