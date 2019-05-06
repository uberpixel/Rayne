//
//  RNOculusAudioSystem.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudioSystem_H_
#define __RAYNE_OculusAudioSystem_H_

#include "RNOculusAudio.h"

namespace RN
{
	class OculusAudioDevice : public Object
	{
	public:
		enum Type
		{
			Input,
			Output
		};

		~OculusAudioDevice() { name->Release(); }

		const Type type;
		const uint32 index;
		const String *name;
		const bool isDefault;

		//TODO:Make private and have protected creator method in audio system
		OculusAudioDevice(Type type, uint32 index, std::string name, bool isDefault) : type(type), index(index), name(RNSTR(name)->Retain()), isDefault(isDefault) { }

		RNDeclareMetaAPI(OculusAudioDevice, OAAPI)
	};

	class OculusAudioWorld;
	class OculusAudioSystem : public Object
	{
	public:
		friend OculusAudioWorld;
		OAAPI ~OculusAudioSystem();
		
		OAAPI virtual Array *GetDevices() = 0;
		OAAPI virtual OculusAudioDevice *GetDefaultInputDevice() = 0;
		OAAPI virtual OculusAudioDevice *GetDefaultOutputDevice() = 0;

		OAAPI virtual void SetOutputDevice(OculusAudioDevice *outputDevice) = 0;
		OAAPI virtual void SetInputDevice(OculusAudioDevice *inputDevice, AudioAsset *targetAsset) = 0;
		
		static OculusAudioSystem *WithInfo(uint32 sampleRate = 48000, uint32 frameSize = 256);
			
	protected:
		OAAPI OculusAudioSystem(uint32 sampleRate, uint32 frameSize);
		std::function<void (void *, void *, unsigned int, unsigned int)> _audioCallback;
		
		void SetAudioCallback(const std::function<void (void *, void*, unsigned int, unsigned int)> &audioCallback){
			_audioCallback = audioCallback;
		}
		
		uint32 _frameSize;
		uint32 _sampleRate;
		
		RNDeclareMetaAPI(OculusAudioSystem, OAAPI)
	};
	
#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
	
	struct OculusAudioSystemRtAudioInternals;
	class OculusAudioSystemRtAudio : public OculusAudioSystem
	{
	public:
		friend OculusAudioSystem;
		
		OAAPI ~OculusAudioSystemRtAudio();
		
		OAAPI Array *GetDevices() final;
		OAAPI OculusAudioDevice *GetDefaultInputDevice() final;
		OAAPI OculusAudioDevice *GetDefaultOutputDevice() final;
		
		OAAPI void SetOutputDevice(OculusAudioDevice *outputDevice) final;
		OAAPI void SetInputDevice(OculusAudioDevice *inputDevice, AudioAsset *targetAsset) final;
		
	private:
		OAAPI OculusAudioSystemRtAudio(uint32 sampleRate, uint32 frameSize);
		static int AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int frameSize, double streamTime, unsigned int status, void *userData);
		
		OculusAudioSystemRtAudioInternals *_internals;
		
		RNDeclareMetaAPI(OculusAudioSystemRtAudio, OAAPI)
	};
#elif RN_PLATFORM_ANDROID
	
#endif
}

#endif /* defined(__RAYNE_OculusAudioSystem_H_) */
