//
//  RNOculusAudioWorld.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudioWORLD_H_
#define __RAYNE_OculusAudioWORLD_H_

#include "RNOculusAudio.h"
#include "RNOculusAudioSource.h"
#include "RNOculusAudioPlayer.h"

namespace RN
{
	class OculusAudioDevice : public Object
	{
	public:
		friend class OculusAudioWorld;
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

	private:
		OculusAudioDevice(Type type, uint32 index, std::string name, bool isDefault) : type(type), index(index), name(RNSTR(name)->Retain()), isDefault(isDefault) { }

		RNDeclareMetaAPI(OculusAudioDevice, OAAPI)
	};

	struct OculusAudioMaterial
	{
		float lowFrequencyAbsorption;
		float midFrequencyAbsorption;
		float highFrequencyAbsorption;

		float scattering;
	};

	struct OculusAudioGeometry
	{
		Mesh *mesh;
		uint32 materialIndex;

		Vector3 position;
		Vector3 scale;
		Quaternion rotation;
	};

	struct OculusAudioWorldInternals;
	class OculusAudioWorld : public SceneAttachment
	{
	public:
		friend class OculusAudioSource;
		friend class OculusAudioPlayer;

		OAAPI static OculusAudioWorld *GetInstance();
		OAAPI static Array *GetDevices();
		OAAPI static OculusAudioDevice *GetDefaultInputDevice();
		OAAPI static OculusAudioDevice *GetDefaultOutputDevice();

		OAAPI OculusAudioWorld(OculusAudioDevice *outputDevice = GetDefaultOutputDevice(), uint8 ambisonicsOrder = 3, uint32 sampleRate = 48000, uint32 frameSize = 480);
		OAAPI ~OculusAudioWorld() override;

		OAAPI void SetOutputDevice(OculusAudioDevice *outputDevice);
		OAAPI void SetInputDevice(OculusAudioDevice *inputDevice, AudioAsset *targetAsset);
			
		OAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; };

		OAAPI OculusAudioPlayer *PlaySound(AudioAsset*resource) const;
		OAAPI OculusAudioSource *PlaySound(AudioAsset *resource, RN::Vector3 position) const;

		OAAPI void AddMaterial(const OculusAudioMaterial &material);
		OAAPI void AddStaticGeometry(const OculusAudioGeometry &geometry);
		OAAPI void UpdateScene();

		OAAPI void *GetBinauralRenderer() const { return _binauralRenderer; }
		OAAPI void *GetEnvironmentalRenderer() const { return _environmentalRenderer; }
		OAAPI void *GetEnvironment() const { return _environment; }
		
		OAAPI void SetCustomWriteCallback(const std::function<void (double)> &customWriteCallback);

		OAAPI void RemoveAudioSource(OculusAudioSource *source) const;

	protected:
		void Update(float delta) override;
			
	private:
		static int AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, unsigned int status, void *userData);
		static OculusAudioWorld *_instance;

		void AddAudioSource(OculusAudioSource *source) const;

		void AddAudioPlayer(OculusAudioPlayer *player) const;
		void RemoveAudioPlayer(OculusAudioPlayer *player) const;

		SceneNode *_listener;

/*		SoundIo *_soundio;
		SoundIoDevice *_inDevice;
		SoundIoDevice *_outDevice;
		SoundIoInStream *_inStream;
		SoundIoOutStream *_outStream;*/

		uint8 _ambisonicsOrder;

		bool _isUpdatingScene;
		void *_scene;
		void *_sceneMesh;
		void *_environment;

		void *_binauralRenderer;
		void *_ambisonicsBinauralEffect;

		void *_environmentalRenderer;

		AudioAsset *_inputBuffer;
		Array *_audioSources;
		Array *_audioPlayers;
		uint32 _frameSize;
		uint32 _sampleRate;

		float *_mixedAmbisonicsFrameData0;
		float *_mixedAmbisonicsFrameData1;
		float *_outputFrameData;
		float *_inputFrameData;

		float *_sharedSourceInputFrameData;
		float *_sharedSourceOutputFrameData;

		std::vector<OculusAudioMaterial> _sceneMaterials;
		std::vector<OculusAudioGeometry> _sceneGeometry;
		
		std::function<void (double)> _customWriteCallback;

		OculusAudioWorldInternals *_internals;
			
		RNDeclareMetaAPI(OculusAudioWorld, OAAPI)
	};
}

#endif /* defined(__RAYNE_OculusAudioWORLD_H_) */
