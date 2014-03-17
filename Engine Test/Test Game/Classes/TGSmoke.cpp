//
//  TGSmoke.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGSmoke.h"

namespace TG
{
	RNDefineMeta(Smoke, RN::ParticleEmitter)
	
	class SmokeParticle : public RN::Particle
	{
	public:
		void Update(float delta)
		{
			time += delta;
			lifespan -= delta;
			
			color.a = alphaInterpolator.GetValue(time);
			size = sizeInterpolator.GetValue(time);
			position += velocity*delta;
		}
		
		RN::Vector3 velocity;
		RN::Interpolator<float> alphaInterpolator;
		RN::Interpolator<RN::Vector2> sizeInterpolator;
	};
		
	Smoke::Smoke() :
	_transparency("transparency", 0.5f, &Smoke::GetTransparency, &Smoke::SetTransparency),
	_velocityMin("minimum velocity", RN::Vector3(-0.25f, 0.4f, -0.25f), &Smoke::GetVelocityMin, &Smoke::SetVelocityMin),
	_velocityMax("maximum velocity", RN::Vector3(0.25f, 0.6f, 0.25f), &Smoke::GetVelocityMax, &Smoke::SetVelocityMax),
	_positionMin("minimum position", RN::Vector3(-0.45f), &Smoke::GetPositionMin, &Smoke::SetPositionMin),
	_positionMax("minimum position", RN::Vector3(0.45f), &Smoke::GetPositionMax, &Smoke::SetPositionMax),
	_sizeStart("start size range", RN::Vector2(0.4f, 0.7f), &Smoke::GetSizeStart, &Smoke::SetSizeStart),
	_sizeEnd("end size range", RN::Vector2(4.0f, 7.0f), &Smoke::GetSizeEnd, &Smoke::SetSizeEnd)
	{
		Initialize();
	}
	
	Smoke::Smoke(const Smoke *fire) :
		RN::ParticleEmitter(fire),
	_transparency("transparency", 0.5f, &Smoke::GetTransparency, &Smoke::SetTransparency),
	_velocityMin("minimum velocity", RN::Vector3(-0.25f, 0.4f, -0.25f), &Smoke::GetVelocityMin, &Smoke::SetVelocityMin),
	_velocityMax("maximum velocity", RN::Vector3(0.25f, 0.6f, 0.25f), &Smoke::GetVelocityMax, &Smoke::SetVelocityMax),
	_positionMin("minimum position", RN::Vector3(-0.45f), &Smoke::GetPositionMin, &Smoke::SetPositionMin),
	_positionMax("minimum position", RN::Vector3(0.45f), &Smoke::GetPositionMax, &Smoke::SetPositionMax),
	_sizeStart("start size range", RN::Vector2(0.4f, 0.7f), &Smoke::GetSizeStart, &Smoke::SetSizeStart),
	_sizeEnd("end size range", RN::Vector2(4.0f, 7.0f), &Smoke::GetSizeEnd, &Smoke::SetSizeEnd)
	{
		Initialize();
	}
	
	Smoke::Smoke(RN::Deserializer *deserializer) :
	RN::ParticleEmitter(deserializer),
	_transparency("transparency", 0.5f, &Smoke::GetTransparency, &Smoke::SetTransparency),
	_velocityMin("minimum velocity", RN::Vector3(-0.25f, 0.4f, -0.25f), &Smoke::GetVelocityMin, &Smoke::SetVelocityMin),
	_velocityMax("maximum velocity", RN::Vector3(0.25f, 0.6f, 0.25f), &Smoke::GetVelocityMax, &Smoke::SetVelocityMax),
	_positionMin("minimum position", RN::Vector3(-0.45f), &Smoke::GetPositionMin, &Smoke::SetPositionMin),
	_positionMax("minimum position", RN::Vector3(0.45f), &Smoke::GetPositionMax, &Smoke::SetPositionMax),
	_sizeStart("start size range", RN::Vector2(0.4f, 0.7f), &Smoke::GetSizeStart, &Smoke::SetSizeStart),
	_sizeEnd("end size range", RN::Vector2(4.0f, 7.0f), &Smoke::GetSizeEnd, &Smoke::SetSizeEnd)
	{
		Initialize();
		_transparency = deserializer->DecodeFloat();
		_velocityMin = deserializer->DecodeVector3();
		_velocityMax = deserializer->DecodeVector3();
		_positionMin = deserializer->DecodeVector3();
		_positionMax = deserializer->DecodeVector3();
		_sizeStart = deserializer->DecodeVector2();
		_sizeEnd = deserializer->DecodeVector2();
	}
	
	void Smoke::Serialize(RN::Serializer *serializer)
	{
		ParticleEmitter::Serialize(serializer);
		serializer->EncodeFloat(_transparency);
		serializer->EncodeVector3(_velocityMin);
		serializer->EncodeVector3(_velocityMax);
		serializer->EncodeVector3(_positionMin);
		serializer->EncodeVector3(_positionMax);
		serializer->EncodeVector2(_sizeStart);
		serializer->EncodeVector2(_sizeEnd);
	}
	
	void Smoke::Initialize()
	{
		AddObservables({&_transparency, &_velocityMin, &_velocityMax, &_positionMin, &_positionMax, &_sizeStart, &_sizeEnd});
		
		RN::Material *material = GetMaterial();
		material->AddTexture(RN::Texture::WithFile("textures/smoke.png"));
		
		SetMaxParticles(100);
		SetParticlesPerSecond(5);
	}
	
	RN::Particle *Smoke::CreateParticle()
	{
		SmokeParticle *particle = new SmokeParticle();
		
		particle->lifespan = 10.0f;
		
		particle->velocity = _rng->RandomVector3Range(_velocityMin, _velocityMax);
		particle->position = _rng->RandomVector3Range(_positionMin, _positionMax);
		
		float sizeScale = 1.0f;
		if(!GetIsLocal())
		{
			sizeScale = GetWorldScale().GetMax();
			
			particle->position *= sizeScale;
			particle->position += GetWorldPosition();
			particle->velocity *= sizeScale;
		}
		
		particle->alphaInterpolator.SetDuration(particle->lifespan);
		particle->alphaInterpolator.SetStartValue(_transparency);
		particle->alphaInterpolator.SetEndValue(0.0f);
		
		particle->sizeInterpolator.SetDuration(particle->lifespan);
		particle->sizeInterpolator.SetStartValue(_rng->RandomFloatRange(_sizeStart->x, _sizeStart->y) * sizeScale);
		particle->sizeInterpolator.SetEndValue(_rng->RandomFloatRange(_sizeEnd->x, _sizeEnd->y) * sizeScale);
		
		Update(0.0f);
		
		return particle;
	}
}
