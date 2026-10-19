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
		ResonanceAudioDevice(Type type, std::string name, bool isDefault) :
			type(type), name(RNSTR(name)->Retain()), isDefault(isDefault) {}

		RNDeclareMetaAPI(ResonanceAudioDevice, RAAPI)
	};

	class ResonanceAudioDeviceMiniAudio : public ResonanceAudioDevice
	{
	public:
		~ResonanceAudioDeviceMiniAudio();
		void *deviceID;

		//TODO:Make private and have protected creator method in audio system
		ResonanceAudioDeviceMiniAudio(Type type, void *device, std::string name, bool isDefault) :
			ResonanceAudioDevice(type, name, isDefault), deviceID(device) {}

		RNDeclareMetaAPI(ResonanceAudioDevice, RAAPI)
	};

	class ResonanceAudioSource;
	class ResonanceAudioWorld;
	class ResonanceAudioListenerContext;
	class ResonanceAudioSystem : public Object
	{
	public:
		friend ResonanceAudioSource;
		friend ResonanceAudioWorld;
		RAAPI ~ResonanceAudioSystem();

		RAAPI virtual Array *GetDevices() = 0;
		RAAPI virtual ResonanceAudioDevice *GetDefaultInputDevice() = 0;
		RAAPI virtual ResonanceAudioDevice *GetDefaultOutputDevice() = 0;

		RAAPI virtual void SetOutputDevice(ResonanceAudioDevice *outputDevice) = 0;
		RAAPI virtual void SetInputDevice(ResonanceAudioDevice *inputDevice) = 0;

		// Called on the output thread after mixing, so callbacks must never block
		RAAPI void SetOutputSamplesCallback(std::function<void(const float *, uint32, uint32, uint32)> callback);

		RAAPI static ResonanceAudioSystem *WithInfo(uint32 sampleRate = 48000, uint32 frameSize = 960, uint8 channelCount = 2);

	protected:
		RAAPI ResonanceAudioSystem(uint32 sampleRate, uint32 frameSize, uint8 channelCount);

		void SetOwningWorld(ResonanceAudioWorld *world) { _owningWorld = world; }
		ResonanceAudioListenerContext *CreateListenerContext();
		void RemoveAllListenerContexts();
		ResonanceAudioListenerContext *GetListenerContext() const;
		void PublishListenerContextsSnapshot();

		void RenderAudio(void *outputBuffer, const void *inputBuffer, uint32 frameCount, uint32 sampleRate, uint32 channelCount, uint32 status);

		std::mutex _outputSamplesLock;
		std::function<void(const float *, uint32, uint32, uint32)> _outputSamplesCallback;

		uint32 _frameSize;
		uint32 _sampleRate;
		uint32 _channelCount;

		std::vector<ResonanceAudioListenerContext *> _listenerContexts;
		Array *_listenerContextSnapshots[3] = {};
		std::atomic<uint32> _listenerContextSnapshotIndex {0};
		std::atomic<uint32> _listenerContextSnapshotInUseIndex {0};
		WeakRef<ResonanceAudioWorld> _owningWorld;

		RNDeclareMetaAPI(ResonanceAudioSystem, RAAPI)
	};

	struct ResonanceAudioSystemMiniAudioInternals;
	class ResonanceAudioSystemMiniAudio : public ResonanceAudioSystem
	{
	public:
		friend ResonanceAudioSystem;

		RAAPI ~ResonanceAudioSystemMiniAudio();

		RAAPI Array *GetDevices() final;
		RAAPI ResonanceAudioDevice *GetDefaultInputDevice() final;
		RAAPI ResonanceAudioDevice *GetDefaultOutputDevice() final;

		RAAPI void SetOutputDevice(ResonanceAudioDevice *outputDevice) final;
		RAAPI void SetInputDevice(ResonanceAudioDevice *inputDevice) final;

	private:
		RAAPI ResonanceAudioSystemMiniAudio(uint32 sampleRate, uint32 frameSize, uint8 channelCount);
		static void AudioCallback(ma_device *pDevice, void *pOutput, const void *pInput, uint32 frameCount);

		RAAPI void UpdateStreamSettings();

		ResonanceAudioSystemMiniAudioInternals *_internals;
		ResonanceAudioDevice *_outputDevice;
		ResonanceAudioDevice *_inputDevice;

		RNDeclareMetaAPI(ResonanceAudioSystemMiniAudio, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioSystem_H_) */
