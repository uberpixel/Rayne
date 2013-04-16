//
//  RNParticleEmitter.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PARTICLEEMITTER_H__
#define __RAYNE_PARTICLEEMITTER_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNParticle.h"
#include "RNMaterial.h"
#include "RNMesh.h"
#include "RNTexture.h"
#include "RNRandom.h"

namespace RN
{
	class ParticleMaterial : public Material
	{
	public:
		ParticleMaterial();
		virtual ~ParticleMaterial();
		
		Vector3 velocity;
		Vector3 velocityChange;
		
		float lifespan;
		float lifespanVariance;
		
		virtual void UpdateParticle(Particle *particle, float delta);
		std::function<void (Particle *, float)> action;
		
		RNDefineMeta(ParticleMaterial, Material)
	};
	
	class ParticleEmitter : public SceneNode
	{
	public:
		ParticleEmitter();
		virtual ~ParticleEmitter();
		
		void Cook(float time, int steps);
		void SetMaterial(ParticleMaterial *material);
		
		void SetSpawnRate(float spawnRate);
		void SetParticlesPerSecond(uint32 particles);
		
		void SetMaxParticles(uint32 maxParticles);
		
		virtual void Update(float delta);
		virtual bool IsVisibleInCamera(Camera *camera);
		virtual void Render(Renderer *renderer, Camera *camera);
		
		virtual Particle *SpawnParticle(ParticleMaterial *material);
		
		Random::LCG lcg;
		
	private:
		std::vector<Particle *> _particles;
		uint32 _maxParticles;
		
		ParticleMaterial *_material;
		Mesh *_mesh;
		
		float _spawnRate;
		float _accDelta;
		
		RNDefineMeta(ParticleEmitter, SceneNode)
	};
}

#endif /* __RAYNE_PARTICLEEMITTER_H__ */
