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

	// Interleaved stereo samples are borrowed until the output callback returns.
	struct ResonanceAudioRecordingBlock
	{
		const float *samples;
		uint32 frameCount;
		uint32 sampleRate;
		uint32 channelCount;
		uint64 firstSampleFrame;
		uint64 streamGeneration;
	};
	using ResonanceAudioRecordingCallback = std::function<void(const ResonanceAudioRecordingBlock &)>;

	class ResonanceAudioWorld;
	class ResonanceAudioListenerContext;
	class ResonanceAudioSystem : public Object
	{
	public:
		friend ResonanceAudioWorld;
		RAAPI ~ResonanceAudioSystem();

		RAAPI virtual Array *GetDevices() = 0;
		RAAPI virtual ResonanceAudioDevice *GetDefaultInputDevice() = 0;
		RAAPI virtual ResonanceAudioDevice *GetDefaultOutputDevice() = 0;

		RAAPI virtual void SetOutputDevice(ResonanceAudioDevice *outputDevice) = 0;
		RAAPI virtual void SetInputDevice(ResonanceAudioDevice *inputDevice) = 0;

		// Called on the output thread after mixing. Callbacks must never block or replace themselves.
		RAAPI void SetOutputSamplesCallback(std::function<void(const float *, uint32, uint32, uint32)> callback);

		RAAPI static ResonanceAudioSystem *WithInfo(uint32 sampleRate = 48000, uint32 frameSize = 960, uint8 channelCount = 2);

	protected:
		using OutputSamplesCallback = std::function<void(const float *, uint32, uint32, uint32)>;

		RAAPI ResonanceAudioSystem(uint32 sampleRate, uint32 frameSize, uint8 channelCount);

		ResonanceAudioListenerContext *CreateListenerContext(ResonanceAudioWorld *world);
		void RemoveAllListenerContexts();
		ResonanceAudioListenerContext *GetListenerContext() const { return _primaryContext.load(std::memory_order_acquire); }
		void SetRecordingContext(ResonanceAudioListenerContext *context);
		void WaitForOutputCallback();

		void RenderAudio(void *outputBuffer, const void *inputBuffer, uint32 frameCount, uint32 sampleRate, uint32 channelCount, uint32 status);
		void AdvanceOutputGeneration() { _outputGeneration.fetch_add(1, std::memory_order_relaxed); }

		std::mutex _outputSamplesLock; // Control threads only.
		std::atomic<OutputSamplesCallback *> _outputSamplesCallback {nullptr};
		std::atomic<ResonanceAudioListenerContext *> _recordingContext {nullptr};
		// Odd while the output thread may dereference either published pointer.
		std::atomic<uint32> _outputCallbackSequence {0};
		std::atomic<uint64> _outputGeneration {0};
		uint64 _nextOutputSampleFrame = 0;

		uint32 _frameSize;
		uint32 _sampleRate;
		uint32 _channelCount;

		std::atomic<ResonanceAudioListenerContext *> _primaryContext {nullptr};

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

		ResonanceAudioSystemMiniAudioInternals *_internals;
		ResonanceAudioDevice *_outputDevice;
		ResonanceAudioDevice *_inputDevice;

		RNDeclareMetaAPI(ResonanceAudioSystemMiniAudio, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioSystem_H_) */
