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
	Particle::Particle(const Vector3& tposition) :
		position(tposition)
	{
		lifespan = 5.0f;
		color = Color(0.0f, 0.0f, 1.0f, 1.0f);
		size = Vector2(0.5f);
	}
	
	Particle::~Particle()
	{
	}
	
	void Particle::Update(float delta)
	{
		lifespan -= delta;
		position += Vector3(0.0f, 1.0f * delta, 0.0f);
	}
}
