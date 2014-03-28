//
//  TGFire.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGFire.h"
#include "TGSmoke.h"

namespace TG
{
	RNDefineMeta(Fire, RN::ParticleEmitter)
	
	class FireParticle : public RN::Particle
	{
	public:
		void Update(float delta)
		{
			time += delta;
			lifespan -= delta;
			
			color.a = alphaInterpolator.GetValue(time);
			size = sizeInterpolator.GetValue(time);
			position += velocity*color.a*1.0f*delta+(target-position)*(1.0f-color.a*1.0f)*delta*2.0f;
		}
		
		RN::Vector3 velocity;
		RN::Interpolator<float> alphaInterpolator;
		RN::Interpolator<RN::Vector2> sizeInterpolator;
		RN::Vector3 target;
	};
		
	Fire::Fire() :
	_velocityMin("minimum velocity", RN::Vector3(-0.45f, 0.8f, -0.45f), &Fire::GetVelocityMin, &Fire::SetVelocityMin),
	_velocityMax("maximum velocity", RN::Vector3(0.45f, 1.0f, 0.45f), &Fire::GetVelocityMax, &Fire::SetVelocityMax),
	_positionMin("minimum position", RN::Vector3(-0.45f), &Fire::GetPositionMin, &Fire::SetPositionMin),
	_positionMax("minimum position", RN::Vector3(0.45f), &Fire::GetPositionMax, &Fire::SetPositionMax),
	_sizeStart("start size range", RN::Vector2(1.5f, 2.7f), &Fire::GetSizeStart, &Fire::SetSizeStart),
	_sizeEnd("end size range", RN::Vector2(0.4f, 0.7f), &Fire::GetSizeEnd, &Fire::SetSizeEnd),
	_targetHeight("target height range", RN::Vector2(1.0f, 2.0f), &Fire::GetTargetHeight, &Fire::SetTargetHeight)
	{
		Initialize();
	}
	
	Fire::Fire(const Fire *fire) :
	RN::ParticleEmitter(fire),
	_velocityMin("minimum velocity", RN::Vector3(-0.45f, 0.8f, -0.45f), &Fire::GetVelocityMin, &Fire::SetVelocityMin),
	_velocityMax("maximum velocity", RN::Vector3(0.45f, 1.0f, 0.45f), &Fire::GetVelocityMax, &Fire::SetVelocityMax),
	_positionMin("minimum position", RN::Vector3(-0.45f), &Fire::GetPositionMin, &Fire::SetPositionMin),
	_positionMax("minimum position", RN::Vector3(0.45f), &Fire::GetPositionMax, &Fire::SetPositionMax),
	_sizeStart("start size range", RN::Vector2(1.5f, 2.7f), &Fire::GetSizeStart, &Fire::SetSizeStart),
	_sizeEnd("end size range", RN::Vector2(0.4f, 0.7f), &Fire::GetSizeEnd, &Fire::SetSizeEnd),
	_targetHeight("target height range", RN::Vector2(1.0f, 2.0f), &Fire::GetTargetHeight, &Fire::SetTargetHeight)
	{
		Initialize();
	}
	
	Fire::Fire(RN::Deserializer *deserializer) :
	RN::ParticleEmitter(deserializer),
	_velocityMin("minimum velocity", RN::Vector3(-0.45f, 0.8f, -0.45f), &Fire::GetVelocityMin, &Fire::SetVelocityMin),
	_velocityMax("maximum velocity", RN::Vector3(0.45f, 1.0f, 0.45f), &Fire::GetVelocityMax, &Fire::SetVelocityMax),
	_positionMin("minimum position", RN::Vector3(-0.45f), &Fire::GetPositionMin, &Fire::SetPositionMin),
	_positionMax("minimum position", RN::Vector3(0.45f), &Fire::GetPositionMax, &Fire::SetPositionMax),
	_sizeStart("start size range", RN::Vector2(1.5f, 2.7f), &Fire::GetSizeStart, &Fire::SetSizeStart),
	_sizeEnd("end size range", RN::Vector2(0.4f, 0.7f), &Fire::GetSizeEnd, &Fire::SetSizeEnd),
	_targetHeight("target height range", RN::Vector2(1.0f, 2.0f), &Fire::GetTargetHeight, &Fire::SetTargetHeight)
	{
		Initialize(deserializer);
		
		_velocityMin = deserializer->DecodeVector3();
		_velocityMax = deserializer->DecodeVector3();
		_positionMin = deserializer->DecodeVector3();
		_positionMax = deserializer->DecodeVector3();
		_sizeStart = deserializer->DecodeVector2();
		_sizeEnd = deserializer->DecodeVector2();
		_targetHeight = deserializer->DecodeVector2();
	}
	
	void Fire::Serialize(RN::Serializer *serializer)
	{
		ParticleEmitter::Serialize(serializer);
		serializer->EncodeVector3(_velocityMin);
		serializer->EncodeVector3(_velocityMax);
		serializer->EncodeVector3(_positionMin);
		serializer->EncodeVector3(_positionMax);
		serializer->EncodeVector2(_sizeStart);
		serializer->EncodeVector2(_sizeEnd);
		serializer->EncodeVector2(_targetHeight);
	}
	
	void Fire::Initialize(RN::Deserializer *deserializer)
	{
		AddObservables({&_velocityMin, &_velocityMax, &_positionMin, &_positionMax, &_sizeStart, &_sizeEnd, &_targetHeight});
		
		if(!deserializer)
		{
			Smoke *smoke = new Smoke();
			AddChild(smoke);
			smoke->Release();
			
			RN::Light *light = new RN::Light();
			light->SetPosition(RN::Vector3(0.0f, 1.0f, 0.0f));
			light->SetColor(RN::Color(1.0f, 0.5f, 0.1f));
			AddChild(light);
			light->Release();
		}
		
		RN::Material *material = GetMaterial();
		material->AddTexture(RN::Texture::WithFile("textures/fire.png"));
		material->SetBlendMode(RN::Material::BlendMode::One, RN::Material::BlendMode::One);
		
		SetMaxParticles(100);
		SetParticlesPerSecond(20);
	}
	
	RN::Particle *Fire::CreateParticle()
	{
		FireParticle *particle = new FireParticle();
		
		particle->lifespan = 5.0f;
		particle->velocity = _rng->RandomVector3Range(_velocityMin, _velocityMax);
		particle->position = _rng->RandomVector3Range(_positionMin, _positionMax);
		particle->target = RN::Vector3(0.0f, _rng->RandomFloatRange(_targetHeight->x, _targetHeight->y), 0.0f);
		
		float sizeScale = 1.0f;
		if(!GetIsLocal())
		{
			sizeScale = GetWorldScale().GetMax();
			
			particle->position *= sizeScale;
			particle->position += GetWorldPosition();
			particle->velocity *= sizeScale;
			particle->target *= sizeScale;
			particle->target += GetWorldPosition();
		}
		
		particle->alphaInterpolator.SetDuration(particle->lifespan);
		particle->alphaInterpolator.SetStartValue(1.0f);
		particle->alphaInterpolator.SetEndValue(0.0f);
		
		particle->sizeInterpolator.SetDuration(particle->lifespan);
		particle->sizeInterpolator.SetStartValue(_rng->RandomFloatRange(_sizeStart->x, _sizeStart->y) * sizeScale);
		particle->sizeInterpolator.SetEndValue(_rng->RandomFloatRange(_sizeEnd->x, _sizeEnd->y) * sizeScale);
		
		particle->Update(0.0f);
		
		return particle;
	}
}
