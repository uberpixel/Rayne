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
#include "RNSteamAudioPlayer.h"

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

	struct SteamAudioMaterial
	{
		float lowFrequencyAbsorption;
		float midFrequencyAbsorption;
		float highFrequencyAbsorption;

		float scattering;
	};

	struct SteamAudioGeometry
	{
		Mesh *mesh;
		uint32 materialIndex;

		Vector3 position;
		Vector3 scale;
		Quaternion rotation;
	};

	struct SteamAudioWorldInternals;
	class SteamAudioWorld : public SceneAttachment
	{
	public:
		friend class SteamAudioSource;
		friend class SteamAudioPlayer;

		SAAPI static SteamAudioWorld *GetInstance();
		SAAPI static Array *GetDevices();

		SAAPI SteamAudioWorld(SteamAudioDevice *outputDevice = nullptr, uint8 ambisonicsOrder = 3, uint32 sampleRate = 44100, uint32 frameSize = 512);
		SAAPI ~SteamAudioWorld() override;
			
		SAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; };

		SAAPI SteamAudioPlayer *PlaySound(AudioAsset*resource) const;

		void SetIndirectAudio(bool enable) { _doIndirectAudio = enable; }
		void SetDirectAudio(bool enable) { _doDirectAudio = enable; }

		SAAPI void AddMaterial(const SteamAudioMaterial &material);
		SAAPI void AddStaticGeometry(const SteamAudioGeometry &geometry);
		SAAPI void UpdateScene();

		SAAPI void *GetBinauralRenderer() const { return _binauralRenderer; }
		SAAPI void *GetEnvironmentalRenderer() const { return _environmentalRenderer; }

	protected:
		void Update(float delta) override;
			
	private:
		static void WriteCallback(struct SoundIoOutStream *outStream, int minSampleCount, int maxSampleCount);
		static SteamAudioWorld *_instance;

		void AddAudioSource(SteamAudioSource *source) const;
		void RemoveAudioSource(SteamAudioSource *source) const;

		void AddAudioPlayer(SteamAudioPlayer *player) const;
		void RemoveAudioPlayer(SteamAudioPlayer *player) const;

		SceneNode *_listener;

		SoundIo *_soundio;
		SoundIoDevice *_device;
		SoundIoOutStream *_outstream;

		uint8 _ambisonicsOrder;

		bool _isUpdatingScene;
		void *_scene;
		void *_sceneMesh;
		void *_environment;

		void *_binauralRenderer;
		void *_ambisonicsBinauralEffect;

		void *_environmentalRenderer;

		Array *_audioSources;
		Array *_audioPlayers;
		uint32 _frameSize;

		float *_mixedAmbisonicsFrameData0;
		float *_mixedAmbisonicsFrameData1;
		float *_outputFrameData;

		float *_sharedSourceInputFrameData;
		float *_sharedSourceOutputFrameData;

		bool _doIndirectAudio;
		bool _doDirectAudio;

		std::vector<SteamAudioMaterial> _sceneMaterials;
		std::vector<SteamAudioGeometry> _sceneGeometry;

		SteamAudioWorldInternals *_internals;
			
		RNDeclareMetaAPI(SteamAudioWorld, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOWORLD_H_) */
