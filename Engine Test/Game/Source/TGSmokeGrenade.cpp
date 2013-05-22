//
//  TGSmokeGrenade.cpp
//  Game-osx
//
//  Created by Sidney Just on 22.05.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGSmokeGrenade.h"

namespace TG
{
	class SmokeParticle : public RN::Particle
	{
	public:
		virtual void Initialize(RN::ParticleEmitter *emitter) override
		{
			RN::Particle::Initialize(emitter);
		}
	};
	
	class SmokeParticleEmitter : public RN::ParticleEmitter
	{
	public:		
		RN::Particle *CreateParticle() override
		{
			SmokeParticle *particle = new SmokeParticle();
			return particle;
		}
	};
	
	
	SmokeGrenade::SmokeGrenade()
	{
		RN::Texture *texture = RN::Texture::WithFile("textures/smoke.png");
		
		_material = new RN::ParticleMaterial();
		_material->AddTexture(texture);
		_material->blending = true;
		_material->blendSource = GL_ONE;
		_material->blendDestination = GL_ONE_MINUS_SRC_ALPHA;
		_material->minVelocity = RN::Vector3(0.0f, 0.4f, 0.0f);
		_material->maxVelocity = RN::Vector3(0.0f, 4.4f, 0.0f);
		
		_emitter = new SmokeParticleEmitter();
		_emitter->SetMaterial(_material);
		_emitter->SetParticlesPerSecond(60);
		_emitter->SetMaxParticles(1000 * 60);
		_emitter->group = 1;
		
		AttachChild(_emitter);
	}
	
	SmokeGrenade::~SmokeGrenade()
	{
		_material->Release();
		_emitter->Release();
	}
}
