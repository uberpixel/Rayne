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
		virtual void Initialize(RN::ParticleEmitter *emitter, RN::ParticleMaterial *material) override
		{
			RN::Particle::Initialize(emitter, material);
			
			position += emitter->GetGenerator()->RandomVector3Range(RN::Vector3(-0.2f), RN::Vector3(0.2f));
			
			size = 0.01f;
			color.a = 0.5f;
		}
		
		virtual void Update(float delta) override
		{
			RN::Vector3 dir = position-emitter->GetWorldPosition();
			position += dir * delta;
			size += delta * 2.0f;
			
			color.a -= delta * 0.2f;
			
			if(color.a <= 0.0)
				lifespan = 0.0f;
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
//		_material->minVelocity = RN::Vector3(0.0f, 0.4f, 0.0f);
//		_material->maxVelocity = RN::Vector3(0.0f, 4.4f, 0.0f);
		
		_emitter = new SmokeParticleEmitter();
		_emitter->SetMaterial(_material);
		_emitter->SetParticlesPerSecond(5);
		_emitter->SetMaxParticles(1000 * 60);
		_emitter->SetRenderGroup(1);
		
		AttachChild(_emitter);
	}
	
	SmokeGrenade::~SmokeGrenade()
	{
		_material->Release();
		_emitter->Release();
	}
}
