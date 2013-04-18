//
//  TGDustEmitter.h
//  Game-osx
//
//  Created by Sidney Just on 18.04.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __Game__TGDustEmitter__
#define __Game__TGDustEmitter__

#include <Rayne.h>

namespace TG
{
	class DustParticle : public RN::Particle
	{
	public:
		DustParticle();
		
		virtual void Initialize();
		virtual void Update(float delta);
		
	private:
		RN::Interpolator<RN::Color> _colorInterpolator;
	};
	
	class DustEmitter : public RN::ParticleEmitter
	{
	public:
		DustEmitter();
		
		virtual RN::Particle *CreateParticle() override;
		
	private:
		RN::Random::LCG _lcg;
		
		RNDefineMeta(DustEmitter, RN::ParticleEmitter)
	};
}

#endif /* defined(__Game__TGDustEmitter__) */
