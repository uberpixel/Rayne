//
//  TGSmoke.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGSmoke.h"

#define kTGSmokeSpreadX GetGenerator()->RandomFloatRange(-0.45f, 0.45f)*1.0f
#define kTGSmokeSpreadY GetGenerator()->RandomFloatRange(-0.45f, 0.45f)*1.0f
#define kTGSmokeVelocity GetGenerator()->RandomFloatRange(0.8f, 1.0f)*1.0f
#define kTGSmokeSize emitter->GetGenerator()->RandomFloatRange(2.0f, 3.5f)

namespace TG
{
	RNDefineMeta(Smoke, RN::ParticleEmitter)
	
	class SmokeParticle : public RN::Particle
	{
	public:
		void Initialize(RN::ParticleEmitter *emitter, RN::ParticleMaterial *material)
		{
			lifespan = 10.0f;
			
			size = 0.0f;
			color.a = emitter->Downcast<Smoke>()->GetTransparency();
			
			_alphaInterpolator.SetDuration(lifespan);
			_alphaInterpolator.SetStartValue(color.a);
			_alphaInterpolator.SetEndValue(0.0f);
			
			_sizeInterpolator.SetDuration(lifespan);
			_sizeInterpolator.SetStartValue(kTGSmokeSize * 0.2f);
			_sizeInterpolator.SetEndValue(kTGSmokeSize * 2.0f);
			
			Update(0.0f);
		}
		
		void Update(float delta)
		{
			_time += delta;
			lifespan -= delta;
			
			color.a = _alphaInterpolator.GetValue(_time);
			size = _sizeInterpolator.GetValue(_time);
			position += velocity*delta;
		}
		
	private:
		float _time;
		RN::Interpolator<float> _alphaInterpolator;
		RN::Interpolator<RN::Vector2> _sizeInterpolator;
	};
		
	Smoke::Smoke() :
	_transparency("transparency", 0.5f, &Smoke::GetTransparency, &Smoke::SetTransparency)
	{
		Initialize();
	}
	
	Smoke::Smoke(const Smoke *fire) :
		RN::ParticleEmitter(fire),
		_transparency("transparency", 0.5f, &Smoke::GetTransparency, &Smoke::SetTransparency)
	{
		Initialize();
	}
	
	Smoke::Smoke(RN::Deserializer *deserializer) :
	RN::ParticleEmitter(deserializer),
	_transparency("transparency", 0.5f, &Smoke::GetTransparency, &Smoke::SetTransparency)
	{
		Initialize();
		_transparency = deserializer->DecodeFloat();
	}
	
	void Smoke::Serialize(RN::Serializer *serializer)
	{
		ParticleEmitter::Serialize(serializer);
		serializer->EncodeFloat(_transparency);
	}
	
	void Smoke::Initialize()
	{
		AddObservable(&_transparency);
		RN::ParticleMaterial *material = new RN::ParticleMaterial();
		
		material->AddTexture(RN::Texture::WithFile("textures/smoke.png"));
		material->SetBlending(true);
		//material->SetBlendMode(RN::Material::BlendMode::One, RN::Material::BlendMode::One);
		
		SetMaterial(material->Autorelease());
		SetMaxParticles(100);
		SetParticlesPerSecond(5);
	}
	
	RN::Particle *Smoke::CreateParticle()
	{
		SmokeParticle *particle = new SmokeParticle();
		particle->velocity = RN::Vector3(kTGSmokeSpreadX, kTGSmokeVelocity, kTGSmokeSpreadY)*0.5f;
		
		particle->position = GetWorldPosition()+RN::Vector3(kTGSmokeSpreadX, kTGSmokeSpreadX, kTGSmokeSpreadY)*1.0f;
		
		return particle;
	}
	
	void Smoke::Update(float delta)
	{
		RN::ParticleEmitter::Update(delta);
	}
	
	void Smoke::UpdateEditMode(float delta)
	{
		RN::ParticleEmitter::Update(delta);
	}
}
