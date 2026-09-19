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
#include "../Math/RNRandom.h"
#include "../Rendering/RNMaterial.h"
#include "../Rendering/RNMesh.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNTexture.h"
#include "RNSceneNode.h"

namespace RN
{
	struct ParticleData
	{
		Vector3 position;
		float rotation;
		Vector2 size;
		Color color;
	};

	class ParticleEmitter : public SceneNode
	{
	public:
		// Vertex count is fixed at construction, clamped to 3..10 (default: quad).
		// Other shapes enclose the UV circle and may extend UVs outside 0..1.
		RNAPI ParticleEmitter(uint32 particleVertexCount = 4);
		RNAPI ParticleEmitter(const ParticleEmitter *emitter);
		RNAPI ~ParticleEmitter() override;

		RNAPI void Cook(float time, int steps);
		RNAPI void SetMaterial(Material *material);
		RNAPI Material *GetMaterial() const { return _material; }
		RNAPI void SetGenerator(RandomNumberGenerator *generator);

		RNAPI void SetSpawnRate(float spawnRate);
		RNAPI void SetParticlesPerSecond(size_t particles);
		RNAPI void SetMaxParticles(uint32 maxParticles);
		RNAPI void SetMaxParticlesSoft(uint32 maxParticles);

		RNAPI uint32 GetParticleVertexCount() const { return _particleVertexCount; }

		RNAPI float GetSpawnRate() const { return _spawnRate; }
		RNAPI uint32 GetMaxParticles() const { return _maxParticles; }

		RNAPI bool GetIsLocal() const { return _isLocal; }
		RNAPI void SetIsLocal(bool local) { _isLocal = local; }

		RNAPI bool GetIgnoreScale() const { return _ignoreScale; }
		RNAPI void SetIgnoreScale(bool ignore) { _ignoreScale = ignore; }

		RNAPI void SetCanRollParticles(bool canRoll) { _canRollParticles = canRoll; }

		RNAPI bool GetIsSorted() const { return _isSorted; }
		RNAPI void SetIsSorted(bool sorted) { _isSorted = sorted; }

		RNAPI bool GetIsRenderedInversed() const { return _isRenderedInversed; }
		RNAPI void SetIsRenderedInversed(bool renderedInversed) { _isRenderedInversed = renderedInversed; }

		RNAPI RandomNumberGenerator *GetGenerator() const { return _rng; }

		RNAPI void Update(float delta) override;
		RNAPI bool CanRender(Renderer *renderer, Camera *camera) const override;
		RNAPI void Render(Renderer *renderer, Camera *camera) const override;

		RNAPI size_t GetNumParticles() const { return _lifespans.size(); }
		
	protected:		
		RNAPI virtual void UpdateLifespans(float delta);
		RNAPI virtual void RemoveParticles(const std::vector<size_t> &indices);

		RNAPI virtual void UpdateParticles(float delta);

		RNAPI virtual void SpawnParticles(float delta);
		RNAPI virtual size_t GetNumParticlesToSpawn(float delta) const;

		RNAPI virtual void CreateParticles(size_t numParticlesToCreate);

		void UpdateParticleOrigin();

		RNAPI float *GetLifespans() { return _lifespans.data(); }
		RNAPI ParticleData *GetParticleData() { return _particles.data(); }

		template<typename T>
		static void RemoveParticlesImpl(const std::vector<size_t>& indices, std::vector<T>& vector)
		{
			const size_t numIndices = indices.size();
			for(size_t j = numIndices; j > 0; j--)
			{
				const size_t i = indices[j - 1];
				const size_t n = numIndices - j + 1;
				vector[i] = std::move(*(vector.end() - n));
			}
			vector.resize(vector.size() - numIndices);
		}

		RandomNumberGenerator *_rng;

	private:
		void UpdateMesh() const;
		void InitializeParticleGeometry();

		std::vector<float> _lifespans;
		std::vector<ParticleData> _particles;

		std::vector<size_t> _particlesToRemove;

		Drawable *_drawable;
		Material *_material;
		Mesh *_mesh;

		bool _isLocal;
		bool _ignoreScale;
		bool _isSorted;
		bool _isRenderedInversed;
		bool _canRollParticles;
		uint32 _maxParticles;
		uint32 _maxParticlesSoft;
		const uint32 _particleVertexCount;
		Vector2 _particleOutline[10];
		Vector2 _particleUVs[10];
		uint8 _particleIndices[24];
		float _spawnRate;

		float _time;

		PositionType _particleOrigin;
		bool _hasParticleOrigin;

		// mutable so it can change in UpdateMesh
		mutable bool _meshIsInitialized;

		__RNDeclareMetaInternal(ParticleEmitter)
	};

	struct GenericParticleData
	{
		float invStartLifespan;

		float startSize;
		float endSize;

		float startRotation;
		float endRotation;

		Vector3 acceleration;
		Vector3 velocity;
	};

	class GenericParticleEmitter : public ParticleEmitter
	{
	public:
		RNAPI GenericParticleEmitter(uint32 particleVertexCount = 4);
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
		Vector3 GetAccelerationRandomizeMin() const { return _accelerationRandomizeMin; }
		void SetAccelerationRandomizeMin(const Vector3 &accelerationRandomizeMin) { _accelerationRandomizeMin = accelerationRandomizeMin; }
		Vector3 GetAccelerationRandomizeMax() const { return _accelerationRandomizeMax; }
		void SetAccelerationRandomizeMax(const Vector3 &accelerationRandomizeMax) { _accelerationRandomizeMax = accelerationRandomizeMax; }

	protected:
		RNAPI void RemoveParticles(const std::vector<size_t> &indices) override;
		RNAPI void UpdateParticles(float delta) override;
		RNAPI void CreateParticles(size_t numParticlesToCreate) override;

		GenericParticleData *GetGenericParticles() { return _genericParticles.data(); }

	private:
		std::vector<GenericParticleData> _genericParticles;
		
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
		Vector3 _accelerationRandomizeMin;
		Vector3 _accelerationRandomizeMax;

		__RNDeclareMetaInternal(GenericParticleEmitter)
	};
} // namespace RN

#endif /* __RAYNE_PARTICLEEMITTER_H__ */
