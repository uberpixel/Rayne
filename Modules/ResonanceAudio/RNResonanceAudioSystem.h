//
//  RNResonanceAudioSystem.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioSystem_H_
#define __RAYNE_ResonanceAudioSystem_H_

#include "RNResonanceAudio.h"

struct ma_device;

namespace RN
{
	class ResonanceAudioDevice : public Object
	{
	public:
		enum Type
		{
			Input,
			Output
		};

		~ResonanceAudioDevice() { name->Release(); }

		const Type type;
		const String *name;
		const bool isDefault;

		//TODO:Make private and have protected creator method in audio system
		ResonanceAudioDevice(Type type, std::string name, bool isDefault) : type(type), name(RNSTR(name)->Retain()), isDefault(isDefault) { }

		RNDeclareMetaAPI(ResonanceAudioDevice, OAAPI)
	};

	class ResonanceAudioDeviceMiniAudio : public ResonanceAudioDevice
	{
	public:
		~ResonanceAudioDeviceMiniAudio();
		void *deviceID;

		//TODO:Make private and have protected creator method in audio system
		ResonanceAudioDeviceMiniAudio(Type type, void *device, std::string name, bool isDefault) : ResonanceAudioDevice(type, name, isDefault), deviceID(device) { }

		RNDeclareMetaAPI(ResonanceAudioDevice, OAAPI)
	};

	class ResonanceAudioWorld;
	class ResonanceAudioSystem : public Object
	{
	public:
		friend ResonanceAudioWorld;
		OAAPI ~ResonanceAudioSystem();
		
		OAAPI virtual Array *GetDevices() = 0;
		OAAPI virtual ResonanceAudioDevice *GetDefaultInputDevice() = 0;
		OAAPI virtual ResonanceAudioDevice *GetDefaultOutputDevice() = 0;

		OAAPI virtual void SetOutputDevice(ResonanceAudioDevice *outputDevice) = 0;
		OAAPI virtual void SetInputDevice(ResonanceAudioDevice *inputDevice) = 0;
		
		OAAPI static ResonanceAudioSystem *WithInfo(uint32 sampleRate = 48000, uint32 frameSize = 960, uint8 channelCount = 2);
			
	protected:
		OAAPI ResonanceAudioSystem(uint32 sampleRate, uint32 frameSize, uint8 channelCount);
		std::function<void (void *, const void *, unsigned int, unsigned int)> _audioCallback;
		
		void SetAudioCallback(const std::function<void (void *, const void*, unsigned int, unsigned int)> &audioCallback){
			_audioCallback = audioCallback;
		}
		
		uint32 _frameSize;
		uint32 _sampleRate;
		uint32 _channelCount;
		
		RNDeclareMetaAPI(ResonanceAudioSystem, OAAPI)
	};

	struct ResonanceAudioSystemMiniAudioInternals;
	class ResonanceAudioSystemMiniAudio : public ResonanceAudioSystem
	{
	public:
		friend ResonanceAudioSystem;
		
		OAAPI ~ResonanceAudioSystemMiniAudio();
		
		OAAPI Array *GetDevices() final;
		OAAPI ResonanceAudioDevice *GetDefaultInputDevice() final;
		OAAPI ResonanceAudioDevice *GetDefaultOutputDevice() final;
		
		OAAPI void SetOutputDevice(ResonanceAudioDevice *outputDevice) final;
		OAAPI void SetInputDevice(ResonanceAudioDevice *inputDevice) final;
		
	private:
		OAAPI ResonanceAudioSystemMiniAudio(uint32 sampleRate, uint32 frameSize, uint8 channelCount);
		static void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, uint32 frameCount);
		
		OAAPI void UpdateStreamSettings();
		
		ResonanceAudioSystemMiniAudioInternals *_internals;
		ResonanceAudioDevice *_outputDevice;
		ResonanceAudioDevice *_inputDevice;
		
		RNDeclareMetaAPI(ResonanceAudioSystemMiniAudio, OAAPI)
	};
}

#endif /* defined(__RAYNE_ResonanceAudioSystem_H_) */
