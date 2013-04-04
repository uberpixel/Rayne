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
		void CreateWorld();
		
		RN::Camera *_camera;
		RN::Camera *_finalcam;
		
		uint32 _touchRight;
		uint32 _touchLeft;
		
		RN::Entity *_parentBlock;
		RN::Entity *_childBlock;
		
		RN::Entity *_spruce;
		RN::Light *_spotLight;
		
		RN::Skeleton *_girlskeleton;
		RN::Skeleton *_zombieskeleton;
		
		RN::bullet::PhysicsWorld *_physicsAttachment;
	};
}

#endif /* defined(__Game__TGWorld__) */
