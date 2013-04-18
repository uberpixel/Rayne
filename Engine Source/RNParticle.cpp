//
//  RNParticle.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticle.h"
#include "RNRandom.h"

namespace RN
{
	Particle::Particle() :
		size(Vector2(1.0f))
	{
		lifespan = 1.0f;
		time = 0.0f;
	}
	
	Particle::Particle(const Vector3& tposition) :
		position(tposition),
		size(Vector2(1.0f))
	{
		lifespan = 1.0f;
		time = 0.0f;
	}
	
	Particle::~Particle()
	{
	}
	
	
	void Particle::Initialize()
	{
	}
			 
	void Particle::Update(float delta)
	{
		time += delta;
		lifespan -= delta;
		
		position += velocity * delta;
	}
}
