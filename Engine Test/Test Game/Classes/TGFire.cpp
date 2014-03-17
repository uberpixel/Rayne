//
//  TGFire.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGFire.h"
#include "TGSmoke.h"

#define kTGFireSpreadX GetGenerator()->RandomFloatRange(-0.45f, 0.45f)*1.0f
#define kTGFireSpreadY GetGenerator()->RandomFloatRange(-0.45f, 0.45f)*1.0f
#define kTGFireVelocity GetGenerator()->RandomFloatRange(0.8f, 1.0f)*1.0f
#define kTGFireSize emitter->GetGenerator()->RandomFloatRange(2.0f, 3.5f)

namespace TG
{
	RNDefineMeta(Fire, RN::ParticleEmitter)
	
	class FireParticle : public RN::Particle
	{
	public:
		void Initialize(RN::ParticleEmitter *emitter)
		{
			lifespan = 5.0f;
			
			size = 0.0f;
			color.a = 1.0f;
			_target = emitter->GetWorldPosition() + RN::Vector3(0.0f, emitter->GetGenerator()->RandomFloatRange(1.0f, 2.0f), 0.0f);
			
			_alphaInterpolator.SetDuration(lifespan);
			_alphaInterpolator.SetStartValue(1.0f);
			_alphaInterpolator.SetEndValue(0.0f);
			
			_sizeInterpolator.SetDuration(lifespan);
			_sizeInterpolator.SetStartValue(kTGFireSize * 0.2f);
			_sizeInterpolator.SetEndValue(kTGFireSize * 1.0f);
			
			Update(0.0f);
		}
		
		void Update(float delta)
		{
			_time += delta;
			lifespan -= delta;
			
			color.a = _alphaInterpolator.GetValue(_time);
			size = _sizeInterpolator.GetValue(_time);
			position += velocity*color.a*1.0f*delta+(_target-position)*(1.0f-color.a*1.0f)*delta*2.0f;
		}
		
		RN::Vector3 velocity;
		
	private:
		float _time;
		RN::Interpolator<float> _alphaInterpolator;
		RN::Interpolator<RN::Vector2> _sizeInterpolator;
		RN::Vector3 _target;
	};
		
	Fire::Fire()
	{
		Initialize();
	}
	
	Fire::Fire(const Fire *fire) :
		RN::ParticleEmitter(fire)
	{
		Initialize();
	}
	
	Fire::Fire(RN::Deserializer *deserializer) :
	RN::ParticleEmitter(deserializer)
	{
		Initialize();
	}
	
	void Fire::Serialize(RN::Serializer *serializer)
	{
		ParticleEmitter::Serialize(serializer);
	}
	
	void Fire::Initialize()
	{
		Smoke *smoke = new Smoke();
		AddChild(smoke);
		smoke->Release();
		
		RN::Material *material = GetMaterial();
		material->AddTexture(RN::Texture::WithFile("textures/fire.png"));
		material->SetBlendMode(RN::Material::BlendMode::One, RN::Material::BlendMode::One);
		
		SetMaxParticles(100);
		SetParticlesPerSecond(20);
	}
	
	RN::Particle *Fire::CreateParticle()
	{
		FireParticle *particle = new FireParticle();
		particle->Initialize(this);
		particle->velocity = RN::Vector3(kTGFireSpreadX, kTGFireVelocity, kTGFireSpreadY)*0.5f;
		
		particle->position = GetWorldPosition()+RN::Vector3(kTGFireSpreadX, kTGFireSpreadX, kTGFireSpreadY)*1.0f;
		
		return particle;
	}
}
