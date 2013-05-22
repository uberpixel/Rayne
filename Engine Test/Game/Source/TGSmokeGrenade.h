//
//  TGSmokeGrenade.h
//  Game-osx
//
//  Created by Sidney Just on 22.05.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Game_osx__TGSmokeGrenade__
#define __Game_osx__TGSmokeGrenade__

#include <Rayne.h>

namespace TG
{
	class SmokeGrenade : public RN::Entity
	{
	public:
		SmokeGrenade();
		~SmokeGrenade() override;
		
	private:
		RN::ParticleEmitter *_emitter;
		RN::ParticleMaterial *_material;
	};
}

#endif /* defined(__Game_osx__TGSmokeGrenade__) */
