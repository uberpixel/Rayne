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

struct SoundIo;
struct SoundIoDevice;
struct SoundIoOutStream;

namespace RN
{
	class SteamAudioDevice : public Object
	{
	public:
		friend class SteamAudioWorld;
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
		friend class SteamAudioSource;
		SAAPI static SteamAudioWorld *GetInstance();

		SAAPI SteamAudioWorld(SteamAudioDevice *outputDevice = nullptr, uint32 sampleRate = 44100, uint32 frameSize = 512);
		SAAPI ~SteamAudioWorld() override;
			
		SAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; };

		SAAPI void PlaySound(AudioAsset*resource);

		SAAPI static Array *GetDevices();

		SAAPI void *GetBinauralRenderer() const { return _binauralRenderer; }
		SAAPI void *GetEnvironmentalRenderer() const { return _environmentalRenderer; }

	protected:
		void Update(float delta) override;
			
	private:
		static void WriteCallback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
		static SteamAudioWorld *_instance;

		void AddAudioSource(SteamAudioSource *source) const;
		void RemoveAudioSource(SteamAudioSource *source) const;

		SceneNode *_listener;

		SoundIo *_soundio;
		SoundIoDevice *_device;
		SoundIoOutStream *_outstream;

		void *_scene;
		void *_environment;

		void *_binauralRenderer;
		void *_environmentalRenderer;
		void *_ambisonicsBinauralEffect;

		Array *_audioSources;
		uint32 _frameSize;
		float *_ambisonicsFrameData;
		float *_outputFrameData;
			
		RNDeclareMetaAPI(SteamAudioWorld, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOWORLD_H_) */
