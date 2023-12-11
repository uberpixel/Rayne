//
//  RNResonanceAudioWorld.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioWORLD_H_
#define __RAYNE_ResonanceAudioWORLD_H_

#include "RNResonanceAudio.h"
#include "RNResonanceAudioSystem.h"
#include "RNResonanceAudioSource.h"

namespace vraudio {
	class ResonanceAudioApi;
}

namespace RN
{
	enum ResonanceAudioMaterial
	{
		ResonanceAudioMaterialTransparent = 0,
		ResonanceAudioMaterialAcousticCeilingTiles,
		ResonanceAudioMaterialBrickBare,
		ResonanceAudioMaterialBrickPainted,
		ResonanceAudioMaterialConcreteBlockCoarse,
		ResonanceAudioMaterialConcreteBlockPainted,
		ResonanceAudioMaterialCurtainHeavy,
		ResonanceAudioMaterialFiberGlassInsulation,
		ResonanceAudioMaterialGlassThin,
		ResonanceAudioMaterialGlassThick,
		ResonanceAudioMaterialGrass,
		ResonanceAudioMaterialLinoleumOnConcrete,
		ResonanceAudioMaterialMarble,
		ResonanceAudioMaterialMetal,
		ResonanceAudioMaterialParquetOnConcrete,
		ResonanceAudioMaterialPlasterRough,
		ResonanceAudioMaterialPlasterSmooth,
		ResonanceAudioMaterialPlywoodPanel,
		ResonanceAudioMaterialPolishedConcreteOrTile,
		ResonanceAudioMaterialSheetrock,
		ResonanceAudioMaterialWaterOrIceSurface,
		ResonanceAudioMaterialWoodCeiling,
		ResonanceAudioMaterialWoodPanel,
		ResonanceAudioMaterialUniform
	  };

	struct ResonanceAudioWorldInternals;
	class ResonanceAudioWorld : public SceneAttachment
	{
	public:
		friend class ResonanceAudioSource;
		friend class ResonanceAudioPlayer;

		OAAPI static ResonanceAudioWorld *GetInstance();

		OAAPI ResonanceAudioWorld(ResonanceAudioSystem *audioSystem);
		OAAPI ~ResonanceAudioWorld() override;
		
		ResonanceAudioSystem *GetAudioSystem() const { return _audioSystem; }
		
		OAAPI void SetListener(SceneNode *listener);
		SceneNode *GetListener() const { return _listener; };

		OAAPI ResonanceAudioSource *PlaySound(AudioAsset *resource) const;
		OAAPI ResonanceAudioSource *PlaySound(AudioAsset *resource, Vector3 position) const;

		OAAPI void SetRaycastCallback(const std::function<void (Vector3, Vector3, float &)> &raycastCallback);
		OAAPI void SetSimpleRoom(Vector3 position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back);
		OAAPI void SetSimpleRoomEnabled(bool enabled);
		
		OAAPI void SetInputBuffer(AudioAsset *inputBuffer);
		//OAAPI void SetCustomWriteCallback(const std::function<void (double)> &customWriteCallback);

	protected:
		void Update(float delta) override;
			
	private:
		static void AudioCallback(void *outputBuffer, const void *inputBuffer, unsigned int frameSize, unsigned int status);

		void AddAudioSource(ResonanceAudioSource *source);
		void RemoveAudioSource(ResonanceAudioSource* source);
		
		static ResonanceAudioWorld *_instance;

		ResonanceAudioSystem *_audioSystem;
		SceneNode *_listener;

		AudioAsset *_inputBuffer;

		float *_sharedFrameData;

		std::vector<ResonanceAudioSource*> _audioSources;

		std::function<void (Vector3, Vector3, float &)> _raycastCallback;
		//std::function<void (double)> _customWriteCallback;

		vraudio::ResonanceAudioApi *_audioAPI;
			
		RNDeclareMetaAPI(ResonanceAudioWorld, OAAPI)
	};
}

#endif /* defined(__RAYNE_ResonanceAudioWORLD_H_) */
