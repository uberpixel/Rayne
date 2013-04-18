//
//  TGDustEmitter.cpp
//  Game-osx
//
//  Created by Sidney Just on 18.04.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGDustEmitter.h"

namespace TG
{
	RNDeclareMeta(DustEmitter)
	
	DustParticle::DustParticle() :
		_colorInterpolator(RN::InterpolationTypeSinusoidalEaseOut)
	{
	}
	
	void DustParticle::Initialize()
	{
		_colorInterpolator.SetDuration(lifespan);
		_colorInterpolator.SetValues(RN::Color(49, 131, 217), RN::Color(90, 130, 182, 0));
	}
	
	void DustParticle::Update(float delta)
	{
		RN::Particle::Update(delta);
		color = _colorInterpolator.ByValue(time);
	}
	
	
	
	DustEmitter::DustEmitter()
	{
		SetParticlesPerSecond(500);
		SetMaxParticles(100000);
	}
	
	RN::Particle *DustEmitter::CreateParticle()
	{
		DustParticle *particle = new DustParticle();
		particle->position = RN::Vector3(_lcg.RandomFloatRange(-70.0f, 70.0f), _lcg.RandomFloatRange(-20.0f, 80.0f), _lcg.RandomFloatRange(-40.0f, 40.0f));
		return particle;
	}
}
