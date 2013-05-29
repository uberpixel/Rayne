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
#include <RBPhysicsWorld.h>
#include <RBRigidBody.h>
#include <RBKinematicController.h>

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
		
		virtual void Update(float delta);
		
	private:
		void CreateCameras();
		void CreateSponza();
		void CreateForest();
		
		RN::Camera *CreateDownsampleChain(RN::Camera *cam, RN::Shader *shader, int level, RN::TextureParameter::Format format, RN::Shader *firstshader, RN::Texture *tex);
		
		RN::bullet::PhysicsWorld *_physicsAttachment;
		DebugDrawer *_debugAttachment;
		
		ThirdPersonCamera *_camera;
		RN::Camera *_finalcam;
		RN::Light *_spotLight;
		RN::Light *_sunLight;
		RN::Texture *_depthtex;
		RN::Entity *_sponza;
		
		float _exposure;
		float _whitepoint;
		
		Player *_player;
	};
}

#endif /* defined(__Game__TGWorld__) */
