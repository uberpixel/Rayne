//
//  SGWorld.h
//  Sword Game
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __SWORD_GAME_WORLD_H_
#define __SWORD_GAME_WORLD_H_

#include <Rayne.h>

#include "RNOculusCamera.h"
#include "RNBulletWorld.h"
#include "RNBulletRigidBody.h"
#include "RNBulletMaterial.h"

#include "SGPlayer.h"

namespace SG
{
	class World : public RN::Scene
	{
	public:

	protected:
		void WillBecomeActive() override;
		void LoadGround();
		void CreateTestLevel();

		void WillUpdate(float delta) override;

		RN::Entity *_ground;

		RN::Camera *_camera;
		RN::OculusCamera *_oculusCamera;

		RN::BulletWorld *_bulletWorld;
		Player *_player;
	};
}


#endif /* __SWORD_GAME_WORLD_H_ */
