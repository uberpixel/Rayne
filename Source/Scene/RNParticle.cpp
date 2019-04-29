//
//  RNParticle.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticle.h"
#include "RNParticleEmitter.h"

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: Particle
	// ---------------------
	
	Particle::Particle()
	{
		lifespan = 1.0f;
		time = 0.0f;
		storage.size = Vector2(1.0f);
	}
	
	Particle::~Particle()
	{
	}
			 
	void Particle::Update(float delta)
	{
		time += delta;
		lifespan -= delta;
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Generic Particle
	// ---------------------
	
	void GenericParticle::Update(float delta)
	{
		Particle::Update(delta);
		
		storage.color = colorInterpolator.GetValue(time);
		storage.size = sizeInterpolator.GetValue(time);
		
		velocity += gravity*delta;
		storage.position += velocity*delta;
	}
}
