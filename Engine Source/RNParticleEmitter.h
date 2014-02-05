//
//  RNParticleEmitter.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
	class ParticleEmitter;
	class ParticleMaterial : public Material
	{
	public:
		RNAPI ParticleMaterial();
		RNAPI ~ParticleMaterial() override;
		
		Vector3 minVelocity;
		Vector3 maxVelocity;
		
		float lifespan;
		float lifespanVariance;
		
		RNDeclareMetaWithTraits(ParticleMaterial, Material, MetaClassTraitCronstructable)
	};
	
	class ParticleEmitter : public SceneNode
	{
	public:
		RNAPI ParticleEmitter();
		RNAPI ~ParticleEmitter() override;
		
		RNAPI void Cook(float time, int steps);
		RNAPI void SetMaterial(ParticleMaterial *material);
		RNAPI void SetGenerator(RandomNumberGenerator *generator);
		
		RNAPI void SetSpawnRate(float spawnRate);
		RNAPI void SetParticlesPerSecond(size_t particles);
		RNAPI void SetMaxParticles(size_t maxParticles);
		
		RNAPI RandomNumberGenerator *GetGenerator() { return _rng; }
		
		RNAPI void SpawnParticles(size_t particles);
		RNAPI Particle *SpawnParticle();
		
		RNAPI void Update(float delta) override;
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		
	protected:
		RNAPI virtual Particle *CreateParticle();
		RandomNumberGenerator *_rng;
		
	private:
		void UpdateParticles(float delta);
		void UpdateMesh();
		
		std::vector<Particle *> _particles;
		size_t _maxParticles;
		
		ParticleMaterial *_material;
		Mesh *_mesh;
		
		float _spawnRate;
		float _accDelta;
		
		RNDeclareMetaWithTraits(ParticleEmitter, SceneNode, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_PARTICLEEMITTER_H__ */
