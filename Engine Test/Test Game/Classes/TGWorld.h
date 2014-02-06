//
//  TGWorld.h
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Game__TGWorld__
#define __Game__TGWorld__

#include <Rayne.h>

#include "TGPlayer.h"
#include "TGThirdPersonCamera.h"
#include "TGDebugDrawer.h"
#include "TGSmokeGrenade.h"
#include "TGSun.h"
#include "TGCutScene.h"

namespace TG
{
	class World : public RN::World
	{
	public:
		World();
		~World();
		
		void LoadOnThread(RN::Thread *thread) override;
		bool SupportsBackgroundLoading() const override;
		
		virtual void Update(float delta);
		
	private:
		void CreateCameras();
		void ToggleFrameCapturing();
		void RecordFrame();
		void PPActivateBloom(RN::Camera *cam);
		void PPActivateSSAO(RN::Camera *cam);
		void PPActivateFXAA(RN::Camera *cam);
		
		void CreateSponza();
		void CreateForest();
		void CreateGrass();
		void CreateTest();
		void CreateSibenik();
		
		float GetGroundHeight(const RN::Vector3 &position);
		
		bool PositionBlocked(const RN::Vector3 &position);
		
		DebugDrawer *_debugAttachment;
		
		ThirdPersonCamera *_camera;
		RN::Camera *_lightcam;
		RN::Camera *_finalcam;
		RN::Light *_spotLight;
		Sun *_sunLight;
		RN::Texture *_depthtex;
		RN::Entity *_sponza;
		bool _frameCapturing;
		
		RN::PostProcessingPipeline *_refractPipeline;
		
		std::vector<float> _blendmap;
		std::vector<float> _heightMap;
		float _heightBase;
		float _heightExtent;
		std::vector<RN::Entity *> _obstacles;
		
		CutScene *_cutScene;
		
		float _exposure;
		float _whitepoint;
		
		Player *_player;
	};
}

#endif /* defined(__Game__TGWorld__) */
