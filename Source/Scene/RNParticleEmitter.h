//
//  RNParticleEmitter.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PARTICLEEMITTER_H__
#define __RAYNE_PARTICLEEMITTER_H__

#include "../Base/RNBase.h"
#include "RNSceneNode.h"
#include "RNParticle.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNMaterial.h"
#include "../Rendering/RNMesh.h"
#include "../Rendering/RNTexture.h"
#include "../Math/RNRandom.h"

namespace RN
{
	class ParticleEmitter : public SceneNode
	{
	public:
		RNAPI ParticleEmitter();
		RNAPI ParticleEmitter(const ParticleEmitter *emitter);
		RNAPI ~ParticleEmitter() override;
		
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
		
		RNAPI bool GetIsSorted() const { return _isSorted; }
		RNAPI void SetIsSorted(bool sorted) { _isSorted = sorted; }
		
		RNAPI bool GetIsRenderedInversed() const { return _isRenderedInversed; }
		RNAPI void SetIsRenderedInversed(bool renderedInversed) { _isRenderedInversed = renderedInversed; }
		
		RNAPI RandomNumberGenerator *GetGenerator() const { return _rng; }
		
		RNAPI void SpawnParticles(size_t particles);
		RNAPI Particle *SpawnParticle();
		
		RNAPI void Update(float delta) override;
		RNAPI bool CanRender(Renderer *renderer, Camera *camera) const override;
		RNAPI void Render(Renderer *renderer, Camera *camera) const override;
		
	protected:
		RNAPI virtual Particle *CreateParticle();
		RandomNumberGenerator *_rng;
		
	private:
		void UpdateParticles(float delta);
		void UpdateMesh() const;
		
		std::vector<Particle *> _particles;
		
		Drawable *_drawable;
		Material *_material;
		Mesh *_mesh;
		Matrix _transform;
		Quaternion _rotation;
		
		bool _isLocal;
		bool _isSorted;
		bool _isRenderedInversed;
		uint32 _maxParticles;
		float _spawnRate;
		
		float _time;
		
		__RNDeclareMetaInternal(ParticleEmitter)
	};
	
	
	class GenericParticleEmitter : public ParticleEmitter
	{
	public:
		RNAPI GenericParticleEmitter();
		RNAPI GenericParticleEmitter(const GenericParticleEmitter *emitter);
		
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
		Vector2 GetStartRotation() const { return _startRotation; }
		void SetStartRotation(const Vector2 &startRotation) { _startRotation = startRotation; }
		Vector2 GetEndRotation() const { return _endRotation; }
		void SetEndRotation(const Vector2 &endRotation) { _endRotation = endRotation; }
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
		Particle *CreateParticle() override;
		
		Vector2 _lifeSpan;
		Color _startColor;
		Color _endColor;
		Vector2 _startSize;
		Vector2 _endSize;
		Vector2 _startRotation;
		Vector2 _endRotation;
		Vector3 _gravity;
		Vector3 _velocity;
		Vector3 _velocityRandomizeMin;
		Vector3 _velocityRandomizeMax;
		Vector3 _positionRandomizeMin;
		Vector3 _positionRandomizeMax;
		
		__RNDeclareMetaInternal(GenericParticleEmitter)
	};
}

#endif /* __RAYNE_PARTICLEEMITTER_H__ */
