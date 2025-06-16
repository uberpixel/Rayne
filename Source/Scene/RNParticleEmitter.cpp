//
//  RNParticleEmitter.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticleEmitter.h"
#include "../Rendering/RNRenderer.h"

#include "../Debug/RNLogger.h"

namespace RN
{
	RNDefineMeta(ParticleEmitter, SceneNode)
	RNDefineMeta(GenericParticleEmitter, ParticleEmitter)

	// ---------------------
	// MARK: -
	// MARK: Particle Emitter
	// ---------------------

	ParticleEmitter::ParticleEmitter() :
		_material(nullptr),
		_mesh(nullptr),
		_isLocal(true),
		_ignoreScale(false),
		_isSorted(false),
		_isRenderedInversed(false),
		_maxParticles(100),
		_maxParticlesSoft(100),
		_spawnRate(0.05f),
		_canRollParticles(false),
		_time(0.0f),
		_meshIsInitialized(false)
	{
		_rng = new RandomNumberGenerator(RandomNumberGenerator::Type::MersenneTwister);

		SetRenderPriority(RenderPriority::RenderTransparent);

		Shader::Options *shaderOptions = Shader::Options::WithNone();
		shaderOptions->AddDefine("RN_PARTICLES", "1");
		_material = Material::WithShaders(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default), Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default))->Retain();

		shaderOptions->EnableMultiview();
		_material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
		_material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);

		_material->SetDepthWriteEnabled(false);
		_material->SetBlendOperation(BlendOperation::Add, BlendOperation::Max);
		_material->SetBlendFactorSource(BlendFactor::One, BlendFactor::One);
		_material->SetBlendFactorDestination(BlendFactor::OneMinusSourceAlpha, BlendFactor::One);

		SetMaxParticles(_maxParticles);

		Renderer *renderer = Renderer::GetActiveRenderer();
		_drawable = renderer->CreateDrawable();
	}

	ParticleEmitter::ParticleEmitter(const ParticleEmitter *emitter) :
		SceneNode(emitter),
		_material(emitter->GetMaterial()),
		_mesh(nullptr),
		_isLocal(emitter->_isLocal),
		_ignoreScale(emitter->_ignoreScale),
		_isSorted(emitter->_isSorted),
		_isRenderedInversed(emitter->_isRenderedInversed),
		_maxParticles(emitter->_maxParticles),
		_spawnRate(emitter->_spawnRate),
		_time(emitter->_time)
	{
		_rng = emitter->GetGenerator();

		Renderer *renderer = Renderer::GetActiveRenderer();
		_drawable = renderer->CreateDrawable();
	}

	ParticleEmitter::~ParticleEmitter()
	{
		SafeRelease(_material);
		SafeRelease(_mesh);
		SafeRelease(_rng);
	}

	void ParticleEmitter::MakeDirty()
	{
		_drawable->MakeDirty();
	}

	void ParticleEmitter::Cook(float time, int steps)
	{
		float delta = time / steps;

		for(int i = 0; i < steps; i++)
		{
			UpdateParticles(delta);
		}

		UpdateMesh();
	}


	void ParticleEmitter::SetSpawnRate(float spawnRate)
	{
		_spawnRate = spawnRate;
	}

	void ParticleEmitter::SetParticlesPerSecond(size_t particles)
	{
		if(particles == 0)
		{
			_spawnRate = -1.0f;
			return;
		}
		_spawnRate = 1.0f / particles;
	}

	void ParticleEmitter::SetMaxParticles(uint32 maxParticles)
	{
		if(_maxParticles == maxParticles && _mesh) return;
		
		RN_ASSERT(maxParticles < 16383, "Maximum number of particles needs to be smaller than 16383!");
		_maxParticles = maxParticles;
		_maxParticlesSoft = maxParticles;
		SafeRelease(_mesh);

		_mesh = new Mesh({Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
						  Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Color0, PrimitiveType::Color),
						  Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2),
						  Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector2),
						  Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint16)},
						 maxParticles * 4, maxParticles * 6, true); //Create a streamable mesh

		_meshIsInitialized = false;
	}

	void ParticleEmitter::SetMaxParticlesSoft(uint32 maxParticles)
	{
		//This can be used to set a lower limit than the hard maximum, without recreating the mesh, it will only prevent more new particles from being spawned if there are too many, but not remove existing particles
		RN_ASSERT(_maxParticlesSoft <= _maxParticles, "Soft maximum number of particles needs to be smaller than or equal to maximum number of particles!");
		_maxParticlesSoft = maxParticles;
	}

	void ParticleEmitter::SetMaterial(Material *material)
	{
		SafeRelease(_material);
		_material = SafeRetain(material);
	}

	void ParticleEmitter::SetGenerator(RandomNumberGenerator *generator)
	{
		SafeRelease(_rng);
		_rng = SafeRetain(generator);
	}

	void ParticleEmitter::UpdateLifespans(float delta)
	{
		for(float& lifespan : _lifespans)
		{
			lifespan -= delta;
		}

		_particlesToRemove.clear();
		const size_t numParticles = GetNumParticles();
		for(size_t i = 0; i < numParticles; i++)
		{
			if(_lifespans[i] <= 0.0f)
			{
				_particlesToRemove.emplace_back(i);
			}
		}

		if(!_particlesToRemove.empty())
		{
			RemoveParticles(_particlesToRemove);
		}
	}

	void ParticleEmitter::RemoveParticles(const std::vector<size_t> &indices)
	{
		RemoveParticlesImpl(indices, _lifespans);
		RemoveParticlesImpl(indices, _particles);
	}

	void ParticleEmitter::UpdateParticles(float delta)
	{
	}

	void ParticleEmitter::SpawnParticles(float delta)
	{
		if(_spawnRate < k::EpsilonFloat)
			return;

		const size_t numParticlesToSpawn = GetNumParticlesToSpawn(delta);
		_time = fmodf((_time + delta), _spawnRate);

		if(numParticlesToSpawn > 0)
		{
			CreateParticles(numParticlesToSpawn);
		}
	}

	size_t ParticleEmitter::GetNumParticlesToSpawn(float delta) const
	{
		if(_spawnRate < k::EpsilonFloat)
			return 0;

		uint32 spawn = floorf((_time + delta) / _spawnRate);

		if(_maxParticles > 0 && _maxParticlesSoft > 0)
			spawn = std::min(_maxParticlesSoft - static_cast<uint32>(GetNumParticles()), spawn);

		return spawn;
	}

	void ParticleEmitter::CreateParticles(size_t numParticlesToCreate)
	{
		const size_t prevNumParticles = GetNumParticles();
		const size_t newNumParticles = prevNumParticles + numParticlesToCreate;
		
		_lifespans.resize(newNumParticles, 1.0f);

		const Vector3 spawnPosition = GetIsLocal() ? Vector3(0.0f, 0.0f, 0.0f) : GetWorldPosition();
		ParticleData defaultParticle = {spawnPosition, 0.0f, Vector2(1.0f), RN::Color::White()};
		_particles.resize(newNumParticles, defaultParticle);
	}

	void ParticleEmitter::UpdateMesh() const
	{
		if(HasFlags(Flags::Hidden) || GetNumParticles() == 0)
		{
			return;
		}
		
#if RN_PLATFORM_MAC_OS
		_mesh->BeginChanges(false);
#else
		_mesh->BeginChanges(true);
#endif
		Mesh::Chunk chunk = _mesh->GetChunk();

		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		Mesh::ElementIterator<Color> colorIterator = chunk.GetIterator<Color>(Mesh::VertexAttribute::Feature::Color0);
		Mesh::ElementIterator<Vector2> texcoordsIterator = chunk.GetIterator<Vector2>(Mesh::VertexAttribute::Feature::UVCoords0);
		Mesh::ElementIterator<Vector2> sizeIterator = chunk.GetIterator<Vector2>(Mesh::VertexAttribute::Feature::UVCoords1);
		Mesh::ElementIterator<RN::uint16> indexIterator = chunk.GetIterator<RN::uint16>(Mesh::VertexAttribute::Feature::Indices);

		/* HACK: convert the iterators to plain pointers and get their strides
		 * then copy everything by plain memcpy
		 * this circumvents the iterator's complexity and speeds up the copy loop roughly 3x
		 */

		uint8 *vertexPtr = reinterpret_cast<uint8 *>(&*vertexIterator);
		uint8 *colorPtr = reinterpret_cast<uint8 *>(&*colorIterator);
		uint8 *texcoordsPtr = reinterpret_cast<uint8 *>(&*texcoordsIterator);
		uint8 *sizePtr = reinterpret_cast<uint8 *>(&*sizeIterator);
		uint8 *indexPtr = reinterpret_cast<uint8 *>(&*indexIterator);

		const size_t stride = _mesh->GetStride();
		const size_t vertexStride = _mesh->GetVertexPositionsSeparatedSize() > 0 ? _mesh->GetVertexPositionsSeparatedStride() : stride;
		const size_t indexStride = _mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices)->GetSize();

		auto copy1 = [](uint8 *&ptr, size_t stride, const auto &value) {
			memcpy(ptr, &value, sizeof(value));
			ptr += stride;
		};

		auto copy4 = [copy1](uint8 *&ptr, size_t stride, const auto &value) {
			copy1(ptr, stride, value);
			copy1(ptr, stride, value);
			copy1(ptr, stride, value);
			copy1(ptr, stride, value);
		};

		float scale = _ignoreScale ? 1.0f : GetWorldScale().x;

		int stop = std::min(static_cast<int>(GetNumParticles()), static_cast<int>(_maxParticles));
		int start = 0;
		int increment = 1;
		if(_isRenderedInversed)
		{
			start = stop - 1;
			increment = -1;
		}

		if(!_canRollParticles)
		{
			for(int i = start; i >= 0 && i < stop; i += increment)
			{
				const ParticleData &particle = _particles[i];

				copy4(vertexPtr, vertexStride, particle.position);

				copy4(colorPtr, stride, particle.color);

				copy1(texcoordsPtr, stride, Vector2(0.0f, 0.0f));
				copy1(texcoordsPtr, stride, Vector2(1.0f, 0.0f));
				copy1(texcoordsPtr, stride, Vector2(0.0f, 1.0f));
				copy1(texcoordsPtr, stride, Vector2(1.0f, 1.0f));

				Vector2 halfSize = particle.size / 2.0f * scale;
				Vector2 halfDirectionTop;
				halfDirectionTop.x = halfSize.x;
				halfDirectionTop.y = halfSize.y;

				Vector2 halfDirectionBottom;
				halfDirectionBottom.x = halfSize.x;
				halfDirectionBottom.y = -halfSize.y;

				copy1(sizePtr, stride, -halfDirectionTop);
				copy1(sizePtr, stride, halfDirectionBottom);
				copy1(sizePtr, stride, -halfDirectionBottom);
				copy1(sizePtr, stride, halfDirectionTop);

				copy1(indexPtr, indexStride, uint16(i * 4 + 0));
				copy1(indexPtr, indexStride, uint16(i * 4 + 1));
				copy1(indexPtr, indexStride, uint16(i * 4 + 2));
				copy1(indexPtr, indexStride, uint16(i * 4 + 2));
				copy1(indexPtr, indexStride, uint16(i * 4 + 1));
				copy1(indexPtr, indexStride, uint16(i * 4 + 3));
			}
		}
		else
		{
			for(int i = start; i >= 0 && i < stop; i += increment)
			{
				const ParticleData &particle = _particles[i];
				
				copy4(vertexPtr, vertexStride, particle.position);
				
				copy4(colorPtr, stride, particle.color);

				copy1(texcoordsPtr, stride, Vector2(0.0f, 0.0f));
				copy1(texcoordsPtr, stride, Vector2(1.0f, 0.0f));
				copy1(texcoordsPtr, stride, Vector2(0.0f, 1.0f));
				copy1(texcoordsPtr, stride, Vector2(1.0f, 1.0f));

				Vector2 halfSize = particle.size / 2.0f * scale;
				Vector2 halfDirectionTop;
				halfDirectionTop.x = Math::Cos(particle.rotation) * halfSize.x - Math::Sin(particle.rotation) * halfSize.y;
				halfDirectionTop.y = Math::Sin(particle.rotation) * halfSize.x + Math::Cos(particle.rotation) * halfSize.y;

				Vector2 halfDirectionBottom;
				halfDirectionBottom.x = Math::Cos(particle.rotation) * halfSize.x + Math::Sin(particle.rotation) * halfSize.y;
				halfDirectionBottom.y = Math::Sin(particle.rotation) * halfSize.x - Math::Cos(particle.rotation) * halfSize.y;

				copy1(sizePtr, stride, -halfDirectionTop);
				copy1(sizePtr, stride, halfDirectionBottom);
				copy1(sizePtr, stride, -halfDirectionBottom);
				copy1(sizePtr, stride, halfDirectionTop);

				copy1(indexPtr, indexStride, uint16(i * 4 + 0));
				copy1(indexPtr, indexStride, uint16(i * 4 + 1));
				copy1(indexPtr, indexStride, uint16(i * 4 + 2));
				copy1(indexPtr, indexStride, uint16(i * 4 + 2));
				copy1(indexPtr, indexStride, uint16(i * 4 + 1));
				copy1(indexPtr, indexStride, uint16(i * 4 + 3));
			}
		}

		for(uint32 i = stop; i < _maxParticles; i++)
		{
			copy1(indexPtr, indexStride, uint16(i * 4 + 0));
			copy1(indexPtr, indexStride, uint16(i * 4 + 0));
			copy1(indexPtr, indexStride, uint16(i * 4 + 0));
			copy1(indexPtr, indexStride, uint16(i * 4 + 0));
			copy1(indexPtr, indexStride, uint16(i * 4 + 0));
			copy1(indexPtr, indexStride, uint16(i * 4 + 0));
		}

		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		_mesh->changedVertices = true;
		_mesh->changedIndices = true;
		_mesh->EndChanges();

		_meshIsInitialized = true;
	}


	void ParticleEmitter::Update(float delta)
	{
		SceneNode::Update(delta);

		UpdateLifespans(delta);
		UpdateParticles(delta);
		SpawnParticles(delta);

		if(!_isSorted)
			UpdateMesh();
	}

	bool ParticleEmitter::CanRender(Renderer *renderer, Camera *camera) const
	{
		if((GetRenderGroup() & camera->GetRenderGroup()) == 0)
			return false;

		if(HasFlags(Flags::Hidden))
			return false;
		
		if(GetNumParticles() == 0)
			return false;

		if(!_meshIsInitialized && !_isSorted) // sorted meshes get updated after this
			return false;

		//TODO: Add occlusion culling or something
		return true;
	}

	void ParticleEmitter::Render(Renderer *renderer, Camera *camera) const
	{
		SceneNode::Render(renderer, camera);

		if(!_material || GetNumParticles() == 0)
			return;

		if(_isSorted)
		{
			//			std::sort(_particles.begin(), _particles.end(), [camera](Particle *a, Particle *b) { return  (a->storage.position.GetDistance(camera->GetWorldPosition()) < b->storage.position.GetDistance(camera->GetWorldPosition()));});

			UpdateMesh();
		}

		_drawable->Update(_mesh, _material, nullptr, _isLocal ? this : nullptr);
		renderer->SubmitDrawable(_drawable);
	}


	// ---------------------
	// MARK: -
	// MARK: Generic Particle Emitter
	// ---------------------

	GenericParticleEmitter::GenericParticleEmitter() :
		_lifeSpan(Vector2(2.0f, 4.0f)),
		_startColor(Color()),
		_endColor(Color(1.0f, 1.0f, 1.0f, 0.0f)),
		_startSize(Vector2(0.5f, 1.5f)),
		_endSize(Vector2(1.5f, 2.5f)),
		_startRotation(Vector2(0.0f, 0.0f)),
		_endRotation(Vector2(0.0f, 0.0f)),
		_gravity(Vector3(0.0f, -0.1f, 0.0f)),
		_velocity(Vector3(0.0f, 0.5f, 0.0f)),
		_velocityRandomizeMin(Vector3(-0.5f, -0.5f, -0.5f)),
		_velocityRandomizeMax(Vector3(0.5f, 0.5f, 0.5f)),
		_positionRandomizeMin(Vector3(-0.5f, -0.5f, -0.5f)),
		_positionRandomizeMax(Vector3(0.5f, 0.5f, 0.5f)),
		_accelerationRandomizeMin(Vector3(0.0f, 0.0f, 0.0f)),
		_accelerationRandomizeMax(Vector3(0.0f, 0.0f, 0.0f))
	{
	}

	GenericParticleEmitter::GenericParticleEmitter(const GenericParticleEmitter *emitter) :
		ParticleEmitter(emitter),
		_lifeSpan(emitter->_lifeSpan),
		_startColor(emitter->_startColor),
		_endColor(emitter->_endColor),
		_startSize(emitter->_startSize),
		_endSize(emitter->_endSize),
		_startRotation(emitter->_startRotation),
		_endRotation(emitter->_endRotation),
		_gravity(emitter->_gravity),
		_velocity(emitter->_velocity),
		_velocityRandomizeMin(emitter->_velocityRandomizeMin),
		_velocityRandomizeMax(emitter->_velocityRandomizeMax),
		_positionRandomizeMin(emitter->_positionRandomizeMin),
		_positionRandomizeMax(emitter->_positionRandomizeMax),
		_accelerationRandomizeMin(emitter->_accelerationRandomizeMin),
		_accelerationRandomizeMax(emitter->_accelerationRandomizeMax)
	{
	}

	void GenericParticleEmitter::RemoveParticles(const std::vector<size_t>& indices)
	{
		ParticleEmitter::RemoveParticles(indices);
		RemoveParticlesImpl(indices, _genericParticles);
	}

	void GenericParticleEmitter::UpdateParticles(float delta)
	{
		auto lerp = [](auto a, auto b, float t) {
			return a + (b - a) * t;
		};

		ParticleEmitter::UpdateParticles(delta);

		const size_t numParticles = GetNumParticles();
		const float *lifespans = GetLifespans();
		ParticleData *particles = GetParticleData();

		for(size_t i = 0; i < numParticles; i++)
		{
			GenericParticleData &genericData = _genericParticles[i];

			const float t = 1.0f - lifespans[i] * genericData.invStartLifespan;

			particles[i].rotation = lerp(genericData.startRotation, genericData.endRotation, t);
			particles[i].size = Vector2(lerp(genericData.startSize, genericData.endSize, t));
			particles[i].color = lerp(_startColor, _endColor, t);

			genericData.velocity += genericData.acceleration * delta;
			particles[i].position += genericData.velocity * delta;
		}
	}

	void GenericParticleEmitter::CreateParticles(size_t numParticlesToCreate)
	{
		const size_t prevNumParticles = GetNumParticles();

		ParticleEmitter::CreateParticles(numParticlesToCreate);

		const size_t newNumParticles = GetNumParticles();
		
		_genericParticles.resize(newNumParticles);

		float *lifespans = GetLifespans();
		ParticleData *particles = GetParticleData();

		for(size_t i = prevNumParticles; i < newNumParticles; i++)
		{
			ParticleData &particle = particles[i];
			GenericParticleData &genericData = _genericParticles[i];

			lifespans[i] = _rng->GetRandomFloatRange(_lifeSpan.x, _lifeSpan.y);

			genericData.invStartLifespan = lifespans[i] > 0.0f ? 1.0f / lifespans[i] : 0.0f;

			genericData.startSize = _rng->GetRandomFloatRange(_startSize.x, _startSize.y);
			genericData.endSize = _rng->GetRandomFloatRange(_endSize.x, _endSize.y);

			genericData.startRotation = _rng->GetRandomFloatRange(_startRotation.x, _startRotation.y);
			genericData.endRotation = _rng->GetRandomFloatRange(_endRotation.x, _endRotation.y);

			genericData.acceleration = _gravity + _rng->GetRandomVector3Range(_accelerationRandomizeMin, _accelerationRandomizeMax);
			genericData.velocity = _velocity + _rng->GetRandomVector3Range(_velocityRandomizeMin, _velocityRandomizeMax);

			particle.position = _rng->GetRandomVector3Range(_positionRandomizeMin, _positionRandomizeMax);
			particle.rotation = genericData.startRotation;
			particle.size = Vector2(genericData.startSize);
			particle.color = _startColor;
		}

		if(!GetIsLocal())
		{
			const Quaternion rotation = GetWorldRotation();
			for(size_t i = prevNumParticles; i < newNumParticles; i++)
			{
				_genericParticles[i].acceleration = rotation.GetRotatedVector(_genericParticles[i].acceleration);
				_genericParticles[i].velocity = rotation.GetRotatedVector(_genericParticles[i].velocity);
			}
			
			if(!GetIgnoreScale())
			{
				const Vector3 scale = GetWorldScale();
				const float sizeScale = std::max(std::max(scale.x, scale.y), scale.z);
				
				for(size_t i = prevNumParticles; i < newNumParticles; i++)
				{
					_genericParticles[i].acceleration *= scale;
					_genericParticles[i].velocity *= scale;
					_genericParticles[i].startSize *= sizeScale;
					_genericParticles[i].endSize *= sizeScale;
					
					particles[i].position *= scale;
					particles[i].position += GetWorldPosition();
				}
			}
		}
	}
} // namespace RN
