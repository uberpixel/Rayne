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
		_hasEnded(false),
		_ringBufferTemp(nullptr)
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
		
		if(_asset && (_asset->GetType() == AudioAsset::Type::Ringbuffer || _asset->GetType() == AudioAsset::Type::Decoder))
		{
			alDeleteBuffers(3, _ringBuffersID);
		}
		SafeRelease(_asset);
		
		delete[] _ringBufferTemp;
	}

	void OpenALSource::SetAudioAsset(AudioAsset *asset)
	{
		LockGuard<Lockable> lock(_lock);
		
		alSourcei(_source, AL_BUFFER, 0);
		
		if(_asset && (_asset->GetType() == AudioAsset::Type::Ringbuffer || _asset->GetType() == AudioAsset::Type::Decoder))
		{
			alDeleteBuffers(3, _ringBuffersID);
		}

		SafeRelease(_asset);
		_asset = asset;
		SafeRetain(_asset);

		if(!_asset) return;
		
		if(_asset->GetType() == AudioAsset::Type::Static)
		{
			OpenALResourceAttachment *attachment = OpenALResourceAttachment::GetAttachmentForResource(asset);
			alSourcei(_source, AL_BUFFER, attachment->GetBufferID());
		}
		else if(_asset->GetType() == AudioAsset::Type::Ringbuffer || _asset->GetType() == AudioAsset::Type::Decoder)
		{
			alSourcei(_source, AL_LOOPING, AL_FALSE);
			
			alGenBuffers(3, _ringBuffersID);
			
			_ringBufferTemp = new int16[3840];
			std::fill(_ringBufferTemp, _ringBufferTemp + 3840, 0);
			
			//TODO: make the format more flexible
			alBufferData(_ringBuffersID[0], AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
			alBufferData(_ringBuffersID[1], AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
			alBufferData(_ringBuffersID[2], AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
			alSourceQueueBuffers(_source, 3, _ringBuffersID);
		}
		
		if(_asset && _asset->GetType() == AudioAsset::Type::Decoder)
		{
			_asset->Decode();
		}
	}
		
	void OpenALSource::SetRepeat(bool repeat)
	{
		LockGuard<Lockable> lock(_lock);
		
		_isRepeating = repeat;
		
		//It will just keep playing the same buffer if looping streamed stuff
		if(_asset->GetType() == AudioAsset::Type::Ringbuffer || _asset->GetType() == AudioAsset::Type::Decoder) return;
		alSourcei(_source, AL_LOOPING, repeat?AL_TRUE: AL_FALSE);
	}
		
	void OpenALSource::SetPitch(float pitch)
	{
		LockGuard<Lockable> lock(_lock);
		
		alSourcef(_source, AL_PITCH, pitch);
	}
		
	void OpenALSource::SetGain(float gain)
	{
		LockGuard<Lockable> lock(_lock);
		
		alSourcef(_source, AL_GAIN, gain);
	}
		
	void OpenALSource::SetRange(float min, float max)
	{
		LockGuard<Lockable> lock(_lock);
		
		alSourcei(_source, AL_REFERENCE_DISTANCE, min);
		alSourcei(_source, AL_MAX_DISTANCE, max);
	}
		
	void OpenALSource::SetSelfdestruct(bool selfdestruct)
	{
		LockGuard<Lockable> lock(_lock);
		
		_isSelfdestructing = selfdestruct;
	}
		
	void OpenALSource::Play()
	{
		LockGuard<Lockable> lock(_lock);
		
		UpdatePosition(0.0f);

		alSourcePlay(_source);
		_isPlaying = true;
		_hasEnded = false;
	}

	void OpenALSource::Stop()
	{
		LockGuard<Lockable> lock(_lock);
		
		alSourceStop(_source);
		alSourcePause(_source);
        _isPlaying = false;
		
		RNDebug("Stopped: " << (_isPlaying? "true" : "false"));
	}
	
	void OpenALSource::Pause()
	{
		LockGuard<Lockable> lock(_lock);
		
		alSourcePause(_source);
		_isPlaying = false;
		
		RNDebug("Paused: " << (_isPlaying? "true" : "false"));
	}

    void OpenALSource::Seek(float time)
    {
		LockGuard<Lockable> lock(_lock);
		
		if(_asset && _asset->GetType() == AudioAsset::Type::Decoder)
		{
			_asset->Seek(time);
		}
		else
		{
			alSourcef(_source, AL_SEC_OFFSET, time);
		}
    }

	bool OpenALSource::IsPlaying()
	{
		LockGuard<Lockable> lock(_lock);
		return _isPlaying;
	}

	bool OpenALSource::HasEnded()
	{
		LockGuard<Lockable> lock(_lock);
		return _hasEnded;
	}

	bool OpenALSource::IsRepeating()
	{
		LockGuard<Lockable> lock(_lock);
		return _isRepeating;
	}
	
	void OpenALSource::Update(float delta)
	{
		SceneNode::Update(delta);
		
		LockGuard<Lockable> lock(_lock);
		
		if(!_isPlaying) return;
		
		bool hasEnded = false;
		if(_asset && (_asset->GetType() == AudioAsset::Type::Ringbuffer || _asset->GetType() == AudioAsset::Type::Decoder))
		{
			bool isPlaying = _isPlaying;
			if(_asset && _asset->GetType() == AudioAsset::Type::Decoder)
			{
				isPlaying = _asset->Decode();
			}
			
			ALint numberOfProcessedBuffers = 0;
			alGetSourcei(_source, AL_BUFFERS_PROCESSED, &numberOfProcessedBuffers);
			while(numberOfProcessedBuffers > 0)
			{
				uint32 bufferedSamples = _asset->GetBufferedSize() / _asset->GetBytesPerSample();
				if(bufferedSamples >= 3840)
				{
					if(bufferedSamples > 3840*5)
					{
						_asset->PopData(nullptr, _asset->GetBufferedSize() - 2 * 3840 * _asset->GetBytesPerSample());
						RNDebug("too much buffered audio: skipping");
					}
					
					//TODO: Make better and don't hardcode sample type and buffer format and multiple channels
					if(_asset->GetBytesPerSample() / _asset->GetChannels() > 2)
					{
						float samplesBuffer[3840];
						_asset->PopData(samplesBuffer, _asset->GetBytesPerSample()*3840);
						for(size_t i = 0; i < 3840; i++)
						{
							_ringBufferTemp[i] = samplesBuffer[i] * 32000.0f;
						}
					}
					else
					{
						_asset->PopData(_ringBufferTemp, _asset->GetBytesPerSample()*3840);
					}
				}
				else
				{
					if(!isPlaying)
					{
						alSourceStop(_source);
						alSourcePause(_source);
						_isPlaying = false;
						hasEnded = true;
					}
					_asset->PopData(_ringBufferTemp, _asset->GetBufferedSize());
					std::fill(_ringBufferTemp + _asset->GetBufferedSize(), _ringBufferTemp + sizeof(_ringBufferTemp), (ALint)0);
					RNDebug("not enough buffered audio: adding silence");
				}
				
				//TODO: support multiple channels
				ALuint bufferID = 0;
				alSourceUnqueueBuffers(_source, 1, &bufferID);
				alBufferData(bufferID, AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
				alSourceQueueBuffers(_source, 1, &bufferID);
				
				numberOfProcessedBuffers -= 1;
			}
		}
		
		ALenum sourceState = AL_STOPPED;
		alGetSourcei(_source, AL_SOURCE_STATE, &sourceState);
		if((sourceState == AL_STOPPED && _asset && _asset->GetType() == AudioAsset::Type::Static) || hasEnded)
		{
			_isPlaying = false;
			_hasEnded = true;
			if(_isSelfdestructing)
			{
				if(GetSceneInfo())
					GetSceneInfo()->GetScene()->RemoveNode(this);
			}
			else
			{
				if(_asset && _asset->GetType() == AudioAsset::Type::Decoder)
				{
					_asset->Seek(0.0f);
					
					if(_isRepeating)
					{
						_isPlaying = true;
						_hasEnded = false;
						alSourcePlay(_source);
					}
				}
			}
		}
		
		UpdatePosition(delta);
	}

	void OpenALSource::UpdatePosition(float delta)
	{
		Vector3 position = GetWorldPosition();
		alSourcefv(_source, AL_POSITION, &position.x);

		Vector3 velocity = position - _oldPosition;
		_oldPosition = position;

		if(delta == 0.0f)
			return;

		velocity /= delta;
		alSourcefv(_source, AL_VELOCITY, &velocity.x);
	}
}
