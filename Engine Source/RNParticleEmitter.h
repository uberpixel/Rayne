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
	
	class ParticleEmitter : public SceneNode
	{
	public:
		RNAPI ParticleEmitter();
		RNAPI ParticleEmitter(const ParticleEmitter *emitter);
		RNAPI ParticleEmitter(RN::Deserializer *deserializer);
		RNAPI ~ParticleEmitter() override;
		
		RNAPI void Serialize(RN::Serializer *serializer) override;
		
		RNAPI void Cook(float time, int steps);
		RNAPI void SetMaterial(Material *material);
		RNAPI Material *GetMaterial() const { return _material; }
		RNAPI void SetGenerator(RandomNumberGenerator *generator);
		
		RNAPI void SetSpawnRate(float spawnRate);
		RNAPI void SetParticlesPerSecond(size_t particles);
		RNAPI void SetMaxParticles(uint32 maxParticles);
		
		RNAPI float GetSpawnRate() const { return _spawnRate; }
		RNAPI uint32 GetMaxParticles() const { return _maxParticles; }
		
		RNAPI bool GetIsLocal() const { return _isLocal; }
		RNAPI void SetIsLocal(bool local) { _isLocal = local; }
		
		RNAPI RandomNumberGenerator *GetGenerator() const { return _rng; }
		
		RNAPI void SpawnParticles(size_t particles);
		RNAPI Particle *SpawnParticle();
		
		RNAPI void Update(float delta) override;
		RNAPI void UpdateEditMode(float delta) override;
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		
	protected:
		RNAPI virtual Particle *CreateParticle();
		RandomNumberGenerator *_rng;
		
	private:
		void UpdateParticles(float delta);
		void UpdateMesh();
		
		std::vector<Particle *> _particles;
		
		Material *_material;
		Mesh *_mesh;
		Matrix _transform;
		
		RN::Observable<bool, ParticleEmitter> _isLocal;
		RN::Observable<uint32, ParticleEmitter> _maxParticles;
		RN::Observable<float, ParticleEmitter> _spawnRate;
		
		float _time;
		
		RNDeclareMeta(ParticleEmitter)
	};
	
	class GenericParticleEmitter : public ParticleEmitter
	{
	public:
		GenericParticleEmitter();
		GenericParticleEmitter(const GenericParticleEmitter *emitter);
		GenericParticleEmitter(RN::Deserializer *deserializer);
		
		void Serialize(RN::Serializer *serializer) override;
		
		Vector2 GetLifeSpan() const { return _lifeSpan; }
		void SetLifeSpan(const Vector2 &lifeSpan) { _lifeSpan = lifeSpan; }
		Color GetStartColor() const { return _startColor; }
		void SetStartColor(const Color &startColor) { _startColor = startColor; }
		Color GetEndColor() const { return _endColor; }
		void SetEndColor(const Color &endColor) { _endColor = endColor; }
		Vector2 GetStartSize() const { return _startSize; }
		void SetStartSize(const Vector2 &startSize) { _startSize = startSize; }
		Vector2 GetEndSize() const { return _endSize; }
		void SetEndSize(const Vector2 &endSize) { _endSize = endSize; }
		Vector3 GetGravity() const { return _gravity; }
		void SetGravity(const Vector3 &gravity) { _gravity = gravity; }
		Vector3 GetVelocity() const { return _velocity; }
		void SetVelocity(const Vector3 &velocity) { _velocity = velocity; }
		Vector3 GetVelocityRandomizeMin() const { return _velocityRandomizeMin; }
		void SetVelocityRandomizeMin(const Vector3 &velocityRandomizeMin) { _velocityRandomizeMin = velocityRandomizeMin; }
		Vector3 GetVelocityRandomizeMax() const { return _velocityRandomizeMax; }
		void SetVelocityRandomizeMax(const Vector3 &velocityRandomizeMax) { _velocityRandomizeMax = velocityRandomizeMax; }
		Vector3 GetPositionRandomizeMin() const { return _positionRandomizeMin; }
		void SetPositionRandomizeMin(const Vector3 &positionRandomizeMin) { _positionRandomizeMin = positionRandomizeMin; }
		Vector3 GetPositionRandomizeMax() const { return _positionRandomizeMax; }
		void SetPositionRandomizeMax(const Vector3 &positionRandomizeMax) { _positionRandomizeMax = positionRandomizeMax; }
		
	private:
		RN::Particle *CreateParticle();
		void Initialize();
		
		RN::Observable<Vector2, GenericParticleEmitter> _lifeSpan;
		RN::Observable<Color, GenericParticleEmitter> _startColor;
		RN::Observable<Color, GenericParticleEmitter> _endColor;
		RN::Observable<Vector2, GenericParticleEmitter> _startSize;
		RN::Observable<Vector2, GenericParticleEmitter> _endSize;
		RN::Observable<Vector3, GenericParticleEmitter> _gravity;
		RN::Observable<Vector3, GenericParticleEmitter> _velocity;
		RN::Observable<Vector3, GenericParticleEmitter> _velocityRandomizeMin;
		RN::Observable<Vector3, GenericParticleEmitter> _velocityRandomizeMax;
		RN::Observable<Vector3, GenericParticleEmitter> _positionRandomizeMin;
		RN::Observable<Vector3, GenericParticleEmitter> _positionRandomizeMax;
		
		RNDeclareMeta(GenericParticleEmitter)
	};
}

#endif /* __RAYNE_PARTICLEEMITTER_H__ */
