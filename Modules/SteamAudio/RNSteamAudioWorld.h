//
//  RNSteamAudioWorld.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOWORLD_H_
#define __RAYNE_STEAMAUDIOWORLD_H_

#include "RNSteamAudio.h"

#include "RNSteamAudioSource.h"
#include "RNSteamAudioListener.h"

struct SoundIo;
struct SoundIoDevice;
struct SoundIoOutStream;

namespace RN
{
	class SteamAudioDevice : public Object
	{
	public:
		friend SteamAudioWorld;
		enum Type
		{
			Input,
			Output
		};

		~SteamAudioDevice() { name->Release(); id->Release(); }

		const Type type;
		const uint32 index;
		const String *name;
		const String *id;
		const bool isDefault;

	private:
		SteamAudioDevice(Type type, uint32 index, const char *name, const char *id, bool isDefault) : type(type), index(index), name(RNSTR(name)->Retain()), id(RNSTR(id)->Retain()), isDefault(isDefault) { }

		RNDeclareMetaAPI(SteamAudioDevice, SAAPI)
	};

	class SteamAudioWorld : public SceneAttachment
	{
	public:
		SAAPI SteamAudioWorld(SteamAudioDevice *outputDevice = nullptr, uint32 sampleRate = 44100, uint32 frameSize = 512);
		SAAPI ~SteamAudioWorld() override;
			
		SAAPI void SetListener(SteamAudioListener *attachment);
		SAAPI void PlaySound(AudioAsset*resource);

		SAAPI static Array *GetDevices();

		SAAPI void *GetBinauralRenderer() const { return _binauralRenderer; }

	protected:
		void Update(float delta) override;
			
	private:
		SteamAudioListener *_audioListener;

		SoundIo *_soundio;
		SoundIoDevice *_device;
		SoundIoOutStream *_outstream;

		void *_binauralRenderer;
		static void *_ambisonicsBinauralEffect;

		static void WriteCallback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
		static Array *_audioSources;
		static uint32 _frameSize;
		static float *_ambisonicsFrameData;
		static float *_outputFrameData;
			
		RNDeclareMetaAPI(SteamAudioWorld, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOWORLD_H_) */
