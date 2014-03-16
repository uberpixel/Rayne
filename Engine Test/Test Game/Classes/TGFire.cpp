//
//  TGFire.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGFire.h"

#define kTGFireSpreadX GetGenerator()->RandomFloatRange(-0.45f, 0.45f)*0.5f
#define kTGFireSpreadY GetGenerator()->RandomFloatRange(-0.45f, 0.45f)*0.5f
#define kTGFireVelocity GetGenerator()->RandomFloatRange(0.8f, 1.0f)*0.5f
#define kTGFireSize emitter->GetGenerator()->RandomFloatRange(1.0f, 1.5f)

namespace TG
{
	RNDefineMeta(Fire, RN::ParticleEmitter)
	
	class FireParticle : public RN::Particle
	{
	public:
		void Initialize(RN::ParticleEmitter *emitter, RN::ParticleMaterial *material)
		{
			lifespan = 10.0f;//emitter->GetGenerator()->RandomFloatRange(0.1f, 0.3f);
			
			_gravity = 0.0f;
			size = 0.0f;
			color.a = 0.3f;
			
			_alphaInterpolator.SetDuration(lifespan);
			_alphaInterpolator.SetStartValue(0.5f);
			_alphaInterpolator.SetEndValue(0.0f);
			
			_sizeInterpolator.SetDuration(lifespan);
			_sizeInterpolator.SetStartValue(kTGFireSize * 0.2f);
			_sizeInterpolator.SetEndValue(kTGFireSize * 0.4f);
			
			Update(0.0f);
		}
		
		void Update(float delta)
		{
			_time += delta*2.0f;
			
			color.a = _alphaInterpolator.GetValue(_time);
			size = _sizeInterpolator.GetValue(_time);
			
			if(color.a <= RN::k::EpsilonFloat)
				lifespan = 0.0f;
			
			RN::Particle::Update(delta);
		}
		
	private:
		float _gravity;
		float _time;
		RN::Interpolator<float> _alphaInterpolator;
		RN::Interpolator<RN::Vector2> _sizeInterpolator;
	};
		
	Fire::Fire()
	{
		Initialize();
	}
	
	Fire::Fire(RN::Deserializer *deserializer)
	{
		Initialize();
	}
	
	void Fire::Serialize(RN::Serializer *serializer)
	{
		
	}
	
	void Fire::Initialize()
	{
		RN::ParticleMaterial *material = new RN::ParticleMaterial();
		
		material->AddTexture(RN::Texture::WithFile("textures/smoke.png"));
		material->SetBlending(true);
		//material->blendDestination = GL_ONE;
		//material->blendSource = GL_ONE;
		
		SetMaterial(material->Autorelease());
		SetMaxParticles(100);
		SetParticlesPerSecond(20);
	}
	
	RN::Particle *Fire::CreateParticle()
	{
		FireParticle *particle = new FireParticle();
		particle->velocity = RN::Vector3(kTGFireSpreadX, kTGFireVelocity, kTGFireSpreadY)*0.5f;
		
		particle->position = GetWorldPosition()+RN::Vector3(kTGFireSpreadX, kTGFireSpreadX, kTGFireSpreadY)*0.1f;
		
		return particle;
	}
	
	void Fire::Update(float delta)
	{
		RN::ParticleEmitter::Update(delta);
	}
	
	void Fire::UpdateEditMode(float delta)
	{
		RN::ParticleEmitter::Update(delta);
	}
}
