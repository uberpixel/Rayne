//
//  SGPlayer.h
//  Sword Game
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __SWORD_GAME_PLAYER_H_
#define __SWORD_GAME_PLAYER_H_

#include "Rayne.h"
#include "RNBulletKinematicController.h"
#include "RNBulletWorld.h"
#include "RNVRCamera.h"

namespace SG
{
	class World;
	class Player : public RN::SceneNode
	{
	public:
		Player(RN::VRCamera *camera, World *world, RN::BulletWorld *bulletWorld);
		~Player() override;
		
		virtual void Update(float delta) override;
		RN::VRCamera *GetCamera() const { return _camera; }

	private:
		void DidUpdate(ChangeSet changeSet) override;
		
	private:
		void ThrowBox(RN::SceneNode *origin);

		RN::VRCamera *_camera;
		RN::BulletKinematicController *_controller;
		RN::BulletWorld *_bulletWorld;
		World *_world;
		
		RN::Vector3 _cameraRotation;
		RN::Vector3 _feetOffset;
		RN::Vector3 _bodyOffset;
		bool _throwKeyPressed;

		RN::Entity *_hand[2];
		RN::Entity *_bodyEntity;
	};
}

#endif /* defined(__SWORD_GAME_PLAYER_H_) */
