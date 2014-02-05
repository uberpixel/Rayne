//
//  TGPlayer.h
//  Game-osx
//
//  Created by Sidney Just on 06.04.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Game__TGPlayer__
#define __Game__TGPlayer__

#include <Rayne.h>

#include "TGThirdPersonCamera.h"

namespace TG
{
	class Player : public RN::Entity /*RN::bullet::KinematicController*/
	{
	public:
		Player(RN::Model *model);
		virtual ~Player();
		
		void Update(float delta);
		void SetCamera(ThirdPersonCamera *camera) { _camera = camera; }
		
	private:
		ThirdPersonCamera *_camera;
		
		RNDeclareMeta(Player, RN::Entity)
	};
}

#endif /* defined(__Game__TGPlayer__) */
