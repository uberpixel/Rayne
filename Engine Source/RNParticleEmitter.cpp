//
//  RNParticleEmitter.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticleEmitter.h"
#include "RNRenderer.h"

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
	}
	
	ParticleMaterial::~ParticleMaterial()
	{
	}
	
	void ParticleMaterial::UpdateParticle(Particle *particle, float delta)
	{
		if(action)
			action(particle, delta);
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
		
		SetParticlesPerSecond(500);
		SetMaxParticles(10000);
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
			Update(delta);
		}
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
	
	
	Particle *ParticleEmitter::SpawnParticle(ParticleMaterial *material)
	{
		Particle *particle = new Particle(WorldPosition());
		
		particle->position = Vector3(lcg.RandomFloat() * 140.0f - 70.0f, lcg.RandomFloat() * 100.0f-20.0f, lcg.RandomFloat() * 80.0f - 40.0f);
		
		
		particle->color.r = lcg.RandomFloatRange(0, 1);
		particle->color.g = lcg.RandomFloatRange(0, 1);
		particle->color.b = lcg.RandomFloatRange(0, 1);
		
		particle->lifespan = material->lifespan;
		
		return particle;
	}
	
	void ParticleEmitter::Update(float delta)
	{
		SceneNode::Update(delta);
		
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
		
		_accDelta += delta;
		int spawn = floorf(_accDelta / _spawnRate);
		_accDelta = fmodf(_accDelta, _spawnRate);
		
		if(spawn > 0)
		{
			if(_maxParticles > 0)
				spawn = MIN(_maxParticles - (uint32)_particles.size(), spawn);
			
			if(spawn > 0)
			{
				std::vector<Particle *> spawned(spawn);
				
				for(int i=0; i<spawn; i++)
				{
					spawned[i] = SpawnParticle(_material);
				}
				
				_particles.insert(_particles.end(), spawned.begin(), spawned.end());
			}
		}
		
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
	
	
	bool ParticleEmitter::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void ParticleEmitter::Render(Renderer *renderer, Camera *camera)
	{
		RenderingObject object;
		object.mesh = _mesh;
		object.count = static_cast<uint32>(_particles.size());
		object.skeleton = 0;
		object.material = _material;
		object.transform = (Matrix *)&WorldTransform();
		
		renderer->RenderObject(object);
	}
}
