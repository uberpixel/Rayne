//
//  RNOpenALWorld.cpp
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenALWorld.h"

#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

//static LPALCGETSTRINGISOFT alcGetStringiSOFT;

namespace RN
{
	RNDefineMeta(OpenALWorld, SceneAttachment)
		
	OpenALWorld::OpenALWorld(String *outputDeviceName, String *inputDeviceName) :
		_audioListener(nullptr), _outputDevice(nullptr), _inputDevice(nullptr), _inputBuffer(nullptr), _inputBufferTemp(nullptr)
	{
		if(outputDeviceName)
			_outputDevice = alcOpenDevice(outputDeviceName->GetUTF8String());
		else
		    _outputDevice = alcOpenDevice(nullptr);
		if(!_outputDevice)
		{
			RNDebug("rayne-openal: Could not open output audio device.");
			return;
		}
		
		if(inputDeviceName)
		{
			if(!inputDeviceName->IsEqual(RNCSTR("default")))
				_inputDevice = alcCaptureOpenDevice(inputDeviceName->GetUTF8String(), 48000, AL_FORMAT_MONO16, 480);
			else
				_inputDevice = alcCaptureOpenDevice(nullptr, 48000, AL_FORMAT_MONO16, 480);
			if(!_inputDevice)
			{
				RNDebug("rayne-openal: Could not open input audio device.");
			}
			else
			{
				alcCaptureStart(_inputDevice);
				_inputBufferTemp = new int16[10240];
			}
		}

		//Enable HRTF
		int attributes[3] = {ALC_HRTF_SOFT, ALC_TRUE, 0};
			
		_context = alcCreateContext(_outputDevice, attributes);
		alcMakeContextCurrent(_context);
		if(!_context)
		{
			RNDebug("rayne-openal: Could not create audio context.");
			return;
		}

		int hrtf_state = 0;
		alcGetIntegerv(_outputDevice, ALC_HRTF_SOFT, 1, &hrtf_state);
		if(!hrtf_state)
			RNDebug("HRTF not enabled!\n");
		else
		{
			const ALchar *name = alcGetString(_outputDevice, ALC_HRTF_SPECIFIER_SOFT);
			RNDebug("HRTF enabled, using " << name);
		}
	}
		
	OpenALWorld::~OpenALWorld()
	{
		if(_inputDevice)
		{
			alcCaptureStop(_inputDevice);
			alcCaptureCloseDevice(_inputDevice);
		}
		
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(_context);
		alcCloseDevice(_outputDevice);
		
		if(_inputBufferTemp)
		{
			delete[] _inputBufferTemp;
		}
	}

	Array *OpenALWorld::GetOutputDeviceNames()
	{
		const char *bytes = static_cast<const char*>(alcGetString(nullptr, ALC_DEVICE_SPECIFIER));
		Array *devices = new Array();
		String *deviceString = String::WithString(bytes, true);
		while(deviceString->GetLength() > 0)
		{
			devices->AddObject(deviceString);
			bytes += deviceString->GetLength() + 1;
			deviceString = String::WithString(bytes, true);
		}
		
		return devices;
	}

	Array *OpenALWorld::GetInputDeviceNames()
	{
		const char *bytes = static_cast<const char*>(alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER));
		Array *devices = new Array();
		String *deviceString = String::WithString(bytes, true);
		while(deviceString->GetLength() > 0)
		{
			devices->AddObject(deviceString);
			bytes += deviceString->GetLength() + 1;
			deviceString = String::WithString(bytes, true);
		}
		
		return devices;
	}
		
	void OpenALWorld::Update(float delta)
	{
		if(_inputDevice && _inputBuffer)
		{
			ALint sampleCount = 0;
			alcGetIntegerv(_inputDevice, ALC_CAPTURE_SAMPLES, (ALCsizei)sizeof(ALint), &sampleCount);
			alcCaptureSamples(_inputDevice, (ALCvoid *)_inputBufferTemp, sampleCount);

			for(int i = 0; i < sampleCount; i += 1)
			{
				float value = static_cast<float>(_inputBufferTemp[i])/32768.0f;
				_inputBuffer->PushData(&value, 4);
			}
		}
	}

	void OpenALWorld::SetInputAudioAsset(AudioAsset *bufferAsset)
	{
		SafeRelease(_inputBuffer);
		_inputBuffer = SafeRetain(bufferAsset);
	}
		
	void OpenALWorld::SetListener(OpenALListener *attachment)
	{
		if(_audioListener)
			_audioListener->RemoveFromWorld();
			
		_audioListener = attachment;
			
		if(_audioListener)
			_audioListener->InsertIntoWorld(this);
	}
		
	OpenALSource *OpenALWorld::PlaySound(AudioAsset *resource)
	{
		if(_audioListener)
		{
			OpenALSource *source = new OpenALSource(resource);
			_audioListener->GetParent()->AddChild(source);
			source->SetSelfdestruct(true);
			source->Play();
			return source;
		}
		return nullptr;
	}
}
