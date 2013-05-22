//
//  RNParticleEmitter.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticleEmitter.h"
#include "RNRenderer.h"
#include "RNResourcePool.h"

namespace RN
{
	RNDeclareMeta(ParticleMaterial)
	RNDeclareMeta(ParticleEmitter)
	
	// ---------------------
	// MARK: -
	// MARK: Particle Material
	// ---------------------
	
	ParticleMaterial::ParticleMaterial()
	{
		lifespan = 1.0f;
		lifespanVariance = 0.0f;
		
		depthwrite = false;
		
		_rng = new RandomNumberGenerator(RandomNumberGenerator::TypeLCG);
		
		SetShader(ResourcePool::SharedInstance()->ResourceWithName<class Shader>(kRNResourceKeyParticleShader));
	}
	
	ParticleMaterial::~ParticleMaterial()
	{
		_rng->Release();
	}
	
	void ParticleMaterial::InitializeParticle(Particle *particle)
	{
		particle->lifespan = _rng->RandomFloatRange(lifespan, lifespan + lifespanVariance);
		particle->velocity = _rng->RandomVector3Range(minVelocity, maxVelocity);
		
		particle->Initialize();
	}
	
	void ParticleMaterial::SetGenerator(RandomNumberGenerator *generator)
	{
		if(_rng)
			_rng->Release();
		
		_rng = generator->Retain();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Particle Emitter
	// ---------------------
	
	struct ParticleData
	{
		Vector3 position;
		Vector2 size;
		Color color;
	};
	
	ParticleEmitter::ParticleEmitter()
	{
		_mesh = 0;
		_material = 0;
		
		SetParticlesPerSecond(1);
		SetMaxParticles(100);
	}
	
	ParticleEmitter::~ParticleEmitter()
	{
		for(Particle *particle : _particles)
		{
			delete particle;
		}
		
		if(_material)
			_material->Release();
		
		if(_mesh)
			_mesh->Release();
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
	
	void ParticleEmitter::SetParticlesPerSecond(uint32 particles)
	{
		_spawnRate = 1.0f / particles;
	}
	
	void ParticleEmitter::SetMaxParticles(uint32 maxParticles)
	{
		_maxParticles = maxParticles;
		
		if(_mesh)
			_mesh->Release();
		
		MeshDescriptor vertexDescriptor;
		vertexDescriptor.feature = kMeshFeatureVertices;
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementSize   = sizeof(Vector3);
		vertexDescriptor.elementCount  = maxParticles;
		vertexDescriptor.offset        = 0;
		
		MeshDescriptor sizeDescriptor;
		sizeDescriptor.feature = kMeshFeatureUVSet0;
		sizeDescriptor.elementMember = 2;
		sizeDescriptor.elementSize   = sizeof(Vector2);
		sizeDescriptor.elementCount  = maxParticles;
		sizeDescriptor.offset        = sizeof(Vector3);
		
		MeshDescriptor colorDescriptor;
		colorDescriptor.feature = kMeshFeatureColor0;
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementSize   = sizeof(Color);
		colorDescriptor.elementCount  = maxParticles;
		colorDescriptor.offset        = sizeDescriptor.offset + sizeof(Vector2);
		
		Array<MeshDescriptor> descriptors;
		descriptors.AddObject(vertexDescriptor);
		descriptors.AddObject(sizeDescriptor);
		descriptors.AddObject(colorDescriptor);
		
		_mesh = new Mesh(descriptors);
		_mesh->SetVBOUsage(GL_DYNAMIC_DRAW);
		_mesh->SetMode(GL_POINTS);
	}
	
	void ParticleEmitter::SetMaterial(ParticleMaterial *material)
	{
		if(_material)
			_material->Release();
		
		_material = material ? material->Retain() : 0;
	}
	
	
	void ParticleEmitter::SpawnParticles(uint32 particles)
	{
		if(particles == 0)
			return;
		
		std::vector<Particle *> spawned(particles);
		
		for(int i=0; i<particles; i++)
		{
			Particle *particle = CreateParticle();
			_material->InitializeParticle(particle);
			
			spawned[i] = particle;
		}
		
		_particles.insert(_particles.end(), spawned.begin(), spawned.end());
	}
	
	
	Particle *ParticleEmitter::CreateParticle()
	{
		return new Particle(WorldPosition());
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
		
		int spawn = floorf((_accDelta + delta) / _spawnRate);
		_accDelta = fmodf((_accDelta + delta), _spawnRate);
		
		if(spawn > 0)
		{
			if(_maxParticles > 0)
				spawn = MIN(_maxParticles - (uint32)_particles.size(), spawn);
			
			SpawnParticles(spawn);
		}
	}
	
	void ParticleEmitter::UpdateMesh()
	{
		ParticleData *data = _mesh->MeshData<ParticleData>();
		for(Particle *particle : _particles)
		{
			data->position = particle->position;
			data->size     = particle->size;
			data->color    = particle->color;
			
			data ++;
		}
		
		_mesh->UpdateMesh();
	}
	
	
	void ParticleEmitter::Update(float delta)
	{
		SceneNode::Update(delta);
		
		UpdateParticles(delta);
		UpdateMesh();
	}
	
	
	bool ParticleEmitter::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void ParticleEmitter::Render(Renderer *renderer, Camera *camera)
	{
		RenderingObject object;
		object.mesh = _mesh;
		object.count = static_cast<uint32>(_particles.size());
		object.material = _material;
		object.transform = (Matrix *)&WorldTransform();
		
		renderer->RenderObject(object);
	}
}
