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
#include <RBKinematicController.h>

namespace TG
{
	class Player : public RN::bullet::KinematicController
	{
	public:
		Player();
		virtual ~Player();
		
		void Update(float delta);
		
		RNDefineMeta(Player, RN::bullet::KinematicController)
	};
}

#endif /* defined(__Game__TGPlayer__) */
