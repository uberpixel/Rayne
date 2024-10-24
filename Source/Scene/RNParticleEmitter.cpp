//
//  RNParticleEmitter.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticleEmitter.h"
#include "../Rendering/RNRenderer.h"

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
	_isSorted(false),
	_isRenderedInversed(false),
	_maxParticles(100),
	_maxParticlesSoft(100),
	_spawnRate(0.05f),
	_time(0.0f),
	_canRollParticles(false)
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
		_material->SetBlendOperation(BlendOperation::Add, BlendOperation::Add);
		_material->SetBlendFactorSource(BlendFactor::One, BlendFactor::Zero);
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
	_isSorted(emitter->_isSorted),
	_isRenderedInversed(emitter->_isRenderedInversed),
	_maxParticles(emitter->_maxParticles),
	_spawnRate(emitter->_spawnRate)
	{
		_rng = emitter->GetGenerator();
		
		Renderer *renderer = Renderer::GetActiveRenderer();
		_drawable = renderer->CreateDrawable();
	}
	
	ParticleEmitter::~ParticleEmitter()
	{
		for(Particle *particle : _particles)
		{
			delete particle;
		}
		
		SafeRelease(_material);
		SafeRelease(_mesh);
		SafeRelease(_rng);
	}
	
	void ParticleEmitter::Cook(float time, int steps)
	{
		float delta = time / steps;
		
		for(int i=0; i<steps; i++)
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
		RN_ASSERT(maxParticles < 16383, "Maximum number of particles needs to be smaller than 16383!");
		_maxParticles = maxParticles;
		_maxParticlesSoft = maxParticles;
		SafeRelease(_mesh);
		
		_mesh = new Mesh({ Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
			Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Color0, PrimitiveType::Color),
			Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2),
			Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector2),
			Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint16) }, maxParticles*4, maxParticles*6, true); //Create a streamable mesh
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
	
	
	Particle *ParticleEmitter::SpawnParticle()
	{
		Particle *particle = CreateParticle();
		
		_particles.push_back(particle);
		
		return particle;
	}
	
	void ParticleEmitter::SpawnParticles(size_t particles)
	{
		if(particles == 0)
			return;
		
		std::vector<Particle *> spawned;
		spawned.reserve(particles);
		
		for(int i=0; i<particles; i++)
		{
			Particle *particle = CreateParticle();
			if(particle) spawned.push_back(particle);
		}
		
		_particles.insert(_particles.end(), spawned.begin(), spawned.end());
	}
	
	
	Particle *ParticleEmitter::CreateParticle()
	{
		RN_ASSERT(_material, "ParticleEmitter need a material to spawn particles");
		return new Particle();
	}
	
	void ParticleEmitter::UpdateParticles(float delta)
	{
		for(auto i=_particles.begin(); i!=_particles.end();)
		{
			Particle *particle = *i;
			particle->Update(delta);
			
			if(particle->lifespan <= 0.0f)
			{
				delete particle;
				
				i = _particles.erase(i);
				continue;
			}
			
			i ++;
		}
		
		if(_spawnRate < k::EpsilonFloat)
			return;
		
		uint32 spawn = floorf((_time + delta) / _spawnRate);
		_time = fmodf((_time + delta), _spawnRate);
		
		if(spawn > 0)
		{
			if(_maxParticles > 0 && _maxParticlesSoft > 0)
				spawn = std::min(_maxParticlesSoft - static_cast<uint32>(_particles.size()), spawn);
			
			SpawnParticles(spawn);
		}
	}
	
	void ParticleEmitter::UpdateMesh() const
	{
		_mesh->BeginChanges();
		Mesh::Chunk chunk = _mesh->GetChunk();
		
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		Mesh::ElementIterator<Color> colorIterator = chunk.GetIterator<Color>(Mesh::VertexAttribute::Feature::Color0);
		Mesh::ElementIterator<Vector2> texcoordsIterator = chunk.GetIterator<Vector2>(Mesh::VertexAttribute::Feature::UVCoords0);
		Mesh::ElementIterator<Vector2> sizeIterator = chunk.GetIterator<Vector2>(Mesh::VertexAttribute::Feature::UVCoords1);
		Mesh::ElementIterator<RN::uint16> indexIterator = chunk.GetIterator<RN::uint16>(Mesh::VertexAttribute::Feature::Indices);
		
		float scale = GetWorldScale().x;
		
		int stop = std::min(static_cast<int>(_particles.size()), static_cast<int>(_maxParticles));
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
				Particle *particle = _particles[i];
				
				*vertexIterator++ = particle->position;
				*vertexIterator++ = particle->position;
				*vertexIterator++ = particle->position;
				*vertexIterator++ = particle->position;
				
				*colorIterator++ = particle->color;
				*colorIterator++ = particle->color;
				*colorIterator++ = particle->color;
				*colorIterator++ = particle->color;
				
				*texcoordsIterator++ = Vector2(0.0f, 0.0f);
				*texcoordsIterator++ = Vector2(1.0f, 0.0f);
				*texcoordsIterator++ = Vector2(0.0f, 1.0f);
				*texcoordsIterator++ = Vector2(1.0f, 1.0f);
				
				Vector2 halfSize = particle->size / 2.0f * scale;
				Vector2 halfDirectionTop;
				halfDirectionTop.x = halfSize.x;
				halfDirectionTop.y = halfSize.y;
				
				Vector2 halfDirectionBottom;
				halfDirectionBottom.x = halfSize.x;
				halfDirectionBottom.y = -halfSize.y;
				
				*sizeIterator++ = -halfDirectionTop;
				*sizeIterator++ = halfDirectionBottom;
				*sizeIterator++ = -halfDirectionBottom;
				*sizeIterator++ = halfDirectionTop;
				
				*indexIterator++ = i * 4 + 0;
				*indexIterator++ = i * 4 + 1;
				*indexIterator++ = i * 4 + 2;
				*indexIterator++ = i * 4 + 2;
				*indexIterator++ = i * 4 + 1;
				*indexIterator++ = i * 4 + 3;
			}
		}
		else
		{
			for(int i = start; i >= 0 && i < stop; i += increment)
			{
				Particle *particle = _particles[i];
				
				*vertexIterator++ = particle->position;
				*vertexIterator++ = particle->position;
				*vertexIterator++ = particle->position;
				*vertexIterator++ = particle->position;
				
				*colorIterator++ = particle->color;
				*colorIterator++ = particle->color;
				*colorIterator++ = particle->color;
				*colorIterator++ = particle->color;
				
				*texcoordsIterator++ = Vector2(0.0f, 0.0f);
				*texcoordsIterator++ = Vector2(1.0f, 0.0f);
				*texcoordsIterator++ = Vector2(0.0f, 1.0f);
				*texcoordsIterator++ = Vector2(1.0f, 1.0f);
				
				Vector2 halfSize = particle->size / 2.0f * scale;
				Vector2 halfDirectionTop;
				halfDirectionTop.x = Math::Cos(particle->rotation) * halfSize.x - Math::Sin(particle->rotation) * halfSize.y;
				halfDirectionTop.y = Math::Sin(particle->rotation) * halfSize.x + Math::Cos(particle->rotation) * halfSize.y;
				
				Vector2 halfDirectionBottom;
				halfDirectionBottom.x = Math::Cos(particle->rotation) * halfSize.x + Math::Sin(particle->rotation) * halfSize.y;
				halfDirectionBottom.y = Math::Sin(particle->rotation) * halfSize.x - Math::Cos(particle->rotation) * halfSize.y;
				
				*sizeIterator++ = -halfDirectionTop;
				*sizeIterator++ = halfDirectionBottom;
				*sizeIterator++ = -halfDirectionBottom;
				*sizeIterator++ = halfDirectionTop;
				
				*indexIterator++ = i * 4 + 0;
				*indexIterator++ = i * 4 + 1;
				*indexIterator++ = i * 4 + 2;
				*indexIterator++ = i * 4 + 2;
				*indexIterator++ = i * 4 + 1;
				*indexIterator++ = i * 4 + 3;
			}
		}
		
		for(uint32 i = stop; i < _maxParticles; i++)
		{
			*indexIterator++ = i * 4 + 0;
			*indexIterator++ = i * 4 + 0;
			*indexIterator++ = i * 4 + 0;
			*indexIterator++ = i * 4 + 0;
			*indexIterator++ = i * 4 + 0;
			*indexIterator++ = i * 4 + 0;
		}
		
		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		_mesh->changedVertices = true;
		_mesh->changedIndices = true;
		_mesh->EndChanges();
	}
	
	
	void ParticleEmitter::Update(float delta)
	{
		SceneNode::Update(delta);
		
		UpdateParticles(delta);
		
		if(!_isSorted)
			UpdateMesh();
	}
	
	bool ParticleEmitter::CanRender(Renderer *renderer, Camera *camera) const
	{
		//TODO: Add occlusion culling or something
		return true;
	}
	
	void ParticleEmitter::Render(Renderer *renderer, Camera *camera) const
	{
		SceneNode::Render(renderer, camera);
		
		if(!_material || _particles.empty())
			return;
		
		if(_isSorted)
		{
//			std::sort(_particles.begin(), _particles.end(), [camera](Particle *a, Particle *b) { return  (a->storage.position.GetDistance(camera->GetWorldPosition()) < b->storage.position.GetDistance(camera->GetWorldPosition()));});
			
			UpdateMesh();
		}

		_drawable->Update(_mesh, _material, nullptr, _isLocal?this:nullptr);
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
	_positionRandomizeMax(Vector3(0.5f, 0.5f, 0.5f))
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
	_positionRandomizeMax(emitter->_positionRandomizeMax)
	{
		
	}
	
	RN::Particle *GenericParticleEmitter::CreateParticle()
	{
		GenericParticle *particle = new GenericParticle();
		particle->position = _rng->GetRandomVector3Range(_positionRandomizeMin, _positionRandomizeMax);
		
		float lifespan = _rng->GetRandomFloatRange(_lifeSpan.x, _lifeSpan.y);
		particle->lifespan = lifespan;
		
		particle->gravity = _gravity;
		particle->velocity = _velocity + _rng->GetRandomVector3Range(_velocityRandomizeMin, _velocityRandomizeMax);
		
		float sizeScale = 1.0f;
		if(!GetIsLocal())
		{
			Vector3 scale = GetWorldScale();
			sizeScale = std::max(std::max(scale.x, scale.y), scale.z);
			
			particle->position *= scale;
			particle->position += GetWorldPosition();
			particle->gravity *= scale;
			particle->velocity *= scale;
		}
		
		particle->sizeInterpolator.SetStartValue(Vector2(_rng->GetRandomFloatRange(_startSize.x, _startSize.y)) * sizeScale);
		particle->sizeInterpolator.SetEndValue(Vector2(_rng->GetRandomFloatRange(_endSize.x, _endSize.y)) * sizeScale);
		particle->sizeInterpolator.SetDuration(lifespan);
		
		particle->colorInterpolator.SetStartValue(_startColor);
		particle->colorInterpolator.SetEndValue(_endColor);
		particle->colorInterpolator.SetDuration(lifespan);
		
		particle->rotationInterpolator.SetStartValue(Math::DegreesToRadians(_rng->GetRandomFloatRange(_startRotation.x, _startRotation.y)));
		particle->rotationInterpolator.SetEndValue(Math::DegreesToRadians(_rng->GetRandomFloatRange(_endRotation.x, _endRotation.y)));
		particle->rotationInterpolator.SetDuration(lifespan);
		
		particle->Update(0.0f);
		
		return particle;
	}
}
