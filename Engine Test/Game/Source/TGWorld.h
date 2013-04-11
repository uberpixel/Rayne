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

namespace TG
{
	class World : public RN::World
	{
	public:
		World();
		~World();
		
		virtual void Update(float delta);
		virtual void TransformsUpdated();
		
	private:
		void CreateCameras();
		void CreateWorld();
		void CreateForest();
		
		RN::bullet::PhysicsWorld *_physicsAttachment;
		
		ThirdPersonCamera *_camera;
		RN::Camera *_finalcam;
		RN::Light *_spotLight;
		RN::Light *_sunLight;
		
		Player *_player;
	};
}

#endif /* defined(__Game__TGWorld__) */
