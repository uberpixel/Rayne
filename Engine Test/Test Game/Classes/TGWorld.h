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
		void PPActivateBloom(RN::Camera *cam);
		void PPActivateSSAO(RN::Camera *cam);
		void PPActivateFXAA(RN::Camera *cam);
		
		void CreateSponza();
		void CreateForest();
		void CreateGrass();
		void CreateTest();
		void CreateSibenik();
		
		void PlaceEntitiesOnGround(RN::SceneNode *node, RN::SceneNode *ground);
		float GetGroundHeight(const RN::Vector3 &position, RN::SceneNode *ground);
		
		bool PositionBlocked(RN::Vector3 position, RN::Entity **obstacles, int count);
		
		DebugDrawer *_debugAttachment;
		
		ThirdPersonCamera *_camera;
		RN::Camera *_lightcam;
		RN::Camera *_finalcam;
		RN::Light *_spotLight;
		RN::Light *_sunLight;
		RN::Texture *_depthtex;
		RN::Entity *_sponza;
		
		RN::PostProcessingPipeline *_refractPipeline;
		
		float _exposure;
		float _whitepoint;
		
		Player *_player;
	};
}

#endif /* defined(__Game__TGWorld__) */
