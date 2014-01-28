//
//  RNParticleEmitter.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNParticleEmitter.h"
#include "RNRenderer.h"
#include "RNResourceCoordinator.h"

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
		
		blending = true;
		blendSource = BlendMode::One;
		blendDestination = BlendMode::OneMinusSourceAlpha;
		
		depthWrite = false;
		SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyParticleShader, nullptr));
	}
	
	ParticleMaterial::~ParticleMaterial()
	{
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
		_rng = new RandomNumberGenerator(RandomNumberGenerator::Type::LCG);
		
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
		
		if(_rng)
			_rng->Release();
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
		_spawnRate = 1.0f / particles;
	}
	
	void ParticleEmitter::SetMaxParticles(size_t maxParticles)
	{
		_maxParticles = maxParticles;
		
		if(_mesh)
			_mesh->Release();
		
		MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementSize   = sizeof(Vector3);
		vertexDescriptor.offset        = 0;
		
		MeshDescriptor sizeDescriptor(kMeshFeatureUVSet0);
		sizeDescriptor.elementMember = 2;
		sizeDescriptor.elementSize   = sizeof(Vector2);
		sizeDescriptor.offset        = sizeof(Vector3);
		
		MeshDescriptor colorDescriptor(kMeshFeatureColor0);
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementSize   = sizeof(Color);
		colorDescriptor.offset        = sizeDescriptor.offset + sizeof(Vector2);
		
		std::vector<MeshDescriptor> descriptors = { vertexDescriptor, sizeDescriptor, colorDescriptor };
		
		_mesh = new Mesh(descriptors, maxParticles, 0);
		_mesh->SetVBOUsage(GL_DYNAMIC_DRAW);
		_mesh->SetMode(GL_POINTS);
	}
	
	void ParticleEmitter::SetMaterial(ParticleMaterial *material)
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
		particle->Initialize(this, _material);
		
		_particles.push_back(particle);
		
		return particle;
	}
	
	void ParticleEmitter::SpawnParticles(size_t particles)
	{
		if(particles == 0)
			return;
		
		std::vector<Particle *> spawned(particles);
		
		for(int i=0; i<particles; i++)
		{
			Particle *particle = CreateParticle();
			particle->Initialize(this, _material);
			
			spawned[i] = particle;
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
		
		size_t spawn = floorf((_accDelta + delta) / _spawnRate);
		_accDelta = fmodf((_accDelta + delta), _spawnRate);
		
		if(spawn > 0)
		{
			if(_maxParticles > 0)
				spawn = std::min(_maxParticles - _particles.size(), spawn);
			
			SpawnParticles(spawn);
		}
	}
	
	void ParticleEmitter::UpdateMesh()
	{
		Mesh::Chunk chunk = _mesh->GetChunk();
		ParticleData *data = chunk.GetData<ParticleData>();
		
		for(size_t i = 0; i < _particles.size(); i ++)
		{
			if(i >= _maxParticles)
				break;
			
			Particle *particle = _particles[i];
			
			data->position = particle->position;
			data->size     = particle->size;
			data->color    = particle->color;
			
			data ++;
		}
		
		chunk.CommitChanges();
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
		SceneNode::Render(renderer, camera);
		
		if(_particles.empty())
			return;
		
		RenderingObject object;
		FillRenderingObject(object);
		
		object.mesh = _mesh;
		object.count = static_cast<uint32>(_particles.size());
		object.material = _material;
		
		renderer->RenderObject(object);
	}
}
