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

		OAAPI OculusAudioWorld(OculusAudioDevice *outputDevice = GetDefaultOutputDevice(), uint32 sampleRate = 48000, uint32 frameSize = 256, uint32 maxSources = 16);
		OAAPI ~OculusAudioWorld() override;

		OAAPI void SetOutputDevice(OculusAudioDevice *outputDevice);
		OAAPI void SetInputDevice(OculusAudioDevice *inputDevice, AudioAsset *targetAsset);
			
		OAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; };

		OAAPI OculusAudioPlayer *PlaySound(AudioAsset*resource) const;
		OAAPI OculusAudioSource *PlaySound(AudioAsset *resource, RN::Vector3 position) const;
		
		OAAPI void SetSimpleRoom(float width, float height, float depth, float reflectionConstant);

		OAAPI void AddMaterial(const OculusAudioMaterial &material);
		OAAPI void AddStaticGeometry(const OculusAudioGeometry &geometry);
		OAAPI void UpdateScene();
		
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
		
		uint32 RetainSourceIndex();
		void ReleaseSourceIndex(uint32 index);

		SceneNode *_listener;

		AudioAsset *_inputBuffer;
		Array *_audioSources;
		Array *_audioPlayers;
		uint32 _frameSize;
		uint32 _sampleRate;
		uint32 _maxSourceCount;
		
		bool *_isSourceAvailable;

		float *_sharedFrameData;
		float *_tempFrameData;

		bool _isUpdatingScene;
		std::vector<OculusAudioMaterial> _sceneMaterials;
		std::vector<OculusAudioGeometry> _sceneGeometry;
		
		std::function<void (double)> _customWriteCallback;

		OculusAudioWorldInternals *_internals;
			
		RNDeclareMetaAPI(OculusAudioWorld, OAAPI)
	};
}

#endif /* defined(__RAYNE_OculusAudioWORLD_H_) */
