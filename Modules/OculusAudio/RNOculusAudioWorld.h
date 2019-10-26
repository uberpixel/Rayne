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
#include "RNOculusAudioSystem.h"
#include "RNOculusAudioSource.h"
#include "RNOculusAudioPlayer.h"

typedef struct ovrAudioVector3f_ ovrAudioVector3f;

namespace RN
{
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

		OAAPI OculusAudioWorld(OculusAudioSystem *audioSystem, uint32 maxSources = 16);
		OAAPI ~OculusAudioWorld() override;
		
		OculusAudioSystem *GetAudioSystem() const { return _audioSystem; }
		
		OAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; };

		OAAPI OculusAudioPlayer *PlaySound(AudioAsset*resource) const;
		OAAPI OculusAudioSource *PlaySound(AudioAsset *resource, RN::Vector3 position) const;
		
		OAAPI void SetRaycastCallback(const std::function<void (Vector3, Vector3, Vector3 &, Vector3 &)> &raycastCallback);
		OAAPI void SetSimpleRoom(float width, float height, float depth, float reflectionConstant);

		OAAPI void AddMaterial(const OculusAudioMaterial &material);
		OAAPI void AddStaticGeometry(const OculusAudioGeometry &geometry);
		OAAPI void UpdateScene();
		
		OAAPI void SetInputBuffer(AudioAsset *inputBuffer);
		OAAPI void SetCustomWriteCallback(const std::function<void (double)> &customWriteCallback);

	protected:
		void Update(float delta) override;
			
	private:
		static void AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int frameSize, unsigned int status);
		
#if !RN_PLATFORM_ANDROID
		static void RaycastCallback(ovrAudioVector3f origin, ovrAudioVector3f direction, ovrAudioVector3f* hit, ovrAudioVector3f* normal, void* pctx);
#endif
		
		void AddAudioSource(OculusAudioSource *source) const;
		void RemoveAudioSource(OculusAudioSource *source) const;

		void AddAudioPlayer(OculusAudioPlayer *player) const;
		void RemoveAudioPlayer(OculusAudioPlayer *player) const;
		
		uint32 RetainSourceIndex();
		void ReleaseSourceIndex(uint32 index);
		
		static OculusAudioWorld *_instance;

		OculusAudioSystem *_audioSystem;
		SceneNode *_listener;

		AudioAsset *_inputBuffer;
		Array *_audioSources;
		Array *_audioPlayers;
		uint32 _maxSourceCount;
		
		bool *_isSourceAvailable;

		float *_sharedFrameData;
		float *_tempFrameData;
		
		std::function<void (Vector3, Vector3, Vector3 &, Vector3 &)> _raycastCallback;

		std::vector<OculusAudioMaterial> _sceneMaterials;
		std::vector<OculusAudioGeometry> _sceneGeometry;
		
		std::function<void (double)> _customWriteCallback;

#if !RN_PLATFORM_ANDROID
		OculusAudioWorldInternals *_internals;
#endif
			
		RNDeclareMetaAPI(OculusAudioWorld, OAAPI)
	};
}

#endif /* defined(__RAYNE_OculusAudioWORLD_H_) */
