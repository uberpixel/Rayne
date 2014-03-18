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
	RNDefineMeta(ParticleEmitter, SceneNode)
	RNDefineMeta(GenericParticleEmitter, ParticleEmitter)
	
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
	
	ParticleEmitter::ParticleEmitter() :
	_mesh(nullptr),
	_material(nullptr),
	_isLocal("is local", true, &ParticleEmitter::GetIsLocal, &ParticleEmitter::SetIsLocal),
	_isSorted("is sorted", false, &ParticleEmitter::GetIsSorted, &ParticleEmitter::SetIsSorted),
	_isRenderedInversed("is rendered inversed", false, &ParticleEmitter::GetIsRenderedInversed, &ParticleEmitter::SetIsRenderedInversed),
	_maxParticles("max particles", 100, &ParticleEmitter::GetMaxParticles, &ParticleEmitter::SetMaxParticles),
	_spawnRate("Spawn Rate", 0.05f, &ParticleEmitter::GetSpawnRate, &ParticleEmitter::SetSpawnRate)
	{
		AddObservables({&_isLocal, &_isSorted, &_isRenderedInversed, &_maxParticles, &_spawnRate});
		
		_rng = new RandomNumberGenerator(RandomNumberGenerator::Type::LCG);
		
		_material = new Material();
		_material->SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyParticleShader, nullptr));
		
		_material->SetDepthWrite(false);
		_material->SetBlending(true);
		_material->SetBlendMode(RN::Material::BlendMode::One, RN::Material::BlendMode::OneMinusSourceAlpha);
		
		SetMaxParticles(_maxParticles);
	}
	
	ParticleEmitter::ParticleEmitter(const ParticleEmitter *emitter) :
	SceneNode(emitter),
	_mesh(nullptr),
	_material(nullptr),
	_isLocal("Is Local", true, &ParticleEmitter::GetIsLocal, &ParticleEmitter::SetIsLocal),
	_isSorted("is sorted", false, &ParticleEmitter::GetIsSorted, &ParticleEmitter::SetIsSorted),
	_isRenderedInversed("is rendered inversed", false, &ParticleEmitter::GetIsRenderedInversed, &ParticleEmitter::SetIsRenderedInversed),
	_maxParticles("Max Particles", 1, &ParticleEmitter::GetMaxParticles, &ParticleEmitter::SetMaxParticles),
	_spawnRate("Spawn Rate", 0.05f, &ParticleEmitter::GetSpawnRate, &ParticleEmitter::SetSpawnRate)
	{
		AddObservables({&_isLocal, &_isSorted, &_isRenderedInversed, &_maxParticles, &_spawnRate});
		
		_rng = emitter->GetGenerator();
		
		SetMaterial(emitter->GetMaterial());
		SetIsLocal(emitter->GetIsLocal());
		SetSpawnRate(emitter->GetSpawnRate());
		SetMaxParticles(emitter->GetMaxParticles());
	}
	
	ParticleEmitter::ParticleEmitter(RN::Deserializer *deserializer) :
	SceneNode(deserializer),
	_mesh(nullptr),
	_material(nullptr),
	_isLocal("Is Local", true, &ParticleEmitter::GetIsLocal, &ParticleEmitter::SetIsLocal),
	_isSorted("is sorted", false, &ParticleEmitter::GetIsSorted, &ParticleEmitter::SetIsSorted),
	_isRenderedInversed("is rendered inversed", false, &ParticleEmitter::GetIsRenderedInversed, &ParticleEmitter::SetIsRenderedInversed),
	_maxParticles("Max Particles", 1, &ParticleEmitter::GetMaxParticles,&ParticleEmitter::SetMaxParticles),
	_spawnRate("Spawn Rate", 0.05f, &ParticleEmitter::GetSpawnRate, &ParticleEmitter::SetSpawnRate)
	{
		AddObservables({&_isLocal, &_isSorted, &_isRenderedInversed, &_maxParticles, &_spawnRate});
		
		_rng = new RandomNumberGenerator(RandomNumberGenerator::Type::LCG);
		
		SetMaterial(static_cast<Material*>(deserializer->DecodeObject()));
		SetIsLocal(deserializer->DecodeBool());
		SetIsSorted(deserializer->DecodeBool());
		SetIsRenderedInversed(deserializer->DecodeBool());
		SetSpawnRate(deserializer->DecodeFloat());
		SetMaxParticles(static_cast<uint32>(deserializer->DecodeInt32()));
	}
	
	void ParticleEmitter::Serialize(RN::Serializer *serializer)
	{
		SceneNode::Serialize(serializer);
		serializer->EncodeObject(_material);
		serializer->EncodeBool(_isLocal);
		serializer->EncodeBool(_isSorted);
		serializer->EncodeBool(_isRenderedInversed);
		serializer->EncodeFloat(_spawnRate);
		serializer->EncodeInt32(static_cast<int32>(_maxParticles));
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
	
	void ParticleEmitter::SetMaxParticles(uint32 maxParticles)
	{
		_maxParticles = maxParticles;
		
		if(_mesh)
			_mesh->Release();
		
		MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementSize   = sizeof(Vector3);
		vertexDescriptor.offset        = 0;
		
		MeshDescriptor sizeDescriptor(MeshFeature::UVSet0);
		sizeDescriptor.elementMember = 2;
		sizeDescriptor.elementSize   = sizeof(Vector2);
		sizeDescriptor.offset        = sizeof(Vector3);
		
		MeshDescriptor colorDescriptor(MeshFeature::Color0);
		colorDescriptor.elementMember = 4;
		colorDescriptor.elementSize   = sizeof(Color);
		colorDescriptor.offset        = sizeDescriptor.offset + sizeof(Vector2);
		
		std::vector<MeshDescriptor> descriptors = { vertexDescriptor, sizeDescriptor, colorDescriptor };
		
		_mesh = new Mesh(descriptors, maxParticles, 0);
		_mesh->SetVBOUsage(Mesh::MeshUsage::Static);
		_mesh->SetDrawMode(Mesh::DrawMode::Points);
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
		
		std::vector<Particle *> spawned(particles);
		
		for(int i=0; i<particles; i++)
		{
			Particle *particle = CreateParticle();
			
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
		
		uint32 spawn = floorf((_time + delta) / _spawnRate);
		_time = fmodf((_time + delta), _spawnRate);
		
		if(spawn > 0)
		{
			if(_maxParticles > 0)
				spawn = std::min(_maxParticles - static_cast<uint32>(_particles.size()), spawn);
			
			SpawnParticles(spawn);
		}
	}
	
	void ParticleEmitter::UpdateMesh()
	{
		Mesh::Chunk chunk = _mesh->GetChunk();
		ParticleData *data = chunk.GetData<ParticleData>();
		
		int to = std::min(static_cast<int>(_particles.size()), static_cast<int>(_maxParticles));
		if(!_isRenderedInversed)
		{
			for(int i = 0; i < to; i++)
			{
				Particle *particle = _particles[i];
				
				data->position = particle->position;
				data->size     = particle->size;
				data->color    = particle->color;
				
				data ++;
			}
		}
		else
		{
			for(int i = to-1; i >= 0; i--)
			{
				Particle *particle = _particles[i];
				
				data->position = particle->position;
				data->size     = particle->size;
				data->color    = particle->color;
				
				data ++;
			}
		}
		
		chunk.CommitChanges();
	}
	
	
	void ParticleEmitter::Update(float delta)
	{
		SceneNode::Update(delta);
		
		UpdateParticles(delta);
		
		if(!_isSorted)
			UpdateMesh();
	}
	
	void ParticleEmitter::UpdateEditMode(float delta)
	{
		SceneNode::UpdateEditMode(delta);
		
		UpdateParticles(delta);
		if(!_isSorted)
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
		
		if(_isSorted)
		{
			std::sort(_particles.begin(), _particles.end(), [camera](Particle *a, Particle *b) { return  (a->position.GetDistance(camera->GetWorldPosition()) < b->position.GetDistance(camera->GetWorldPosition()));});
			
			UpdateMesh();
		}
		
		RenderingObject object;
		
		if(GetFlags() & Flags::DrawLate)
			object.flags |= RenderingObject::DrawLate;
		
		if(!_isLocal)
		{
			_transform = Matrix::WithIdentity();
			_rotation = Quaternion::WithIdentity();
		}
		else
		{
			_transform = GetWorldTransform();
			_rotation = GetWorldRotation();
		}
		
		object.transform = &_transform;
		object.rotation  = &_rotation;
		object.mesh = _mesh;
		object.count = static_cast<uint32>(_particles.size());
		object.material = _material;
		
		renderer->RenderObject(object);
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Generic Particle Emitter
	// ---------------------
	
	GenericParticleEmitter::GenericParticleEmitter() :
	_startColor("Start Color", Color(), &GenericParticleEmitter::GetStartColor, &GenericParticleEmitter::SetStartColor),
	_endColor("End Color", Color(1.0f, 1.0f, 1.0f, 0.0f), &GenericParticleEmitter::GetEndColor, &GenericParticleEmitter::SetEndColor),
	_gravity("Gravity", Vector3(0.0f, -0.1f, 0.0f), &GenericParticleEmitter::GetGravity, &GenericParticleEmitter::SetGravity),
	_velocity("Velocity", Vector3(0.0f, 0.5f, 0.0f), &GenericParticleEmitter::GetVelocity, &GenericParticleEmitter::SetVelocity),
	_velocityRandomizeMin("Velocity Randomize Min", Vector3(-0.5f), &GenericParticleEmitter::GetVelocityRandomizeMin, &GenericParticleEmitter::SetVelocityRandomizeMin),
	_velocityRandomizeMax("Velocity Randomize Max", Vector3(0.5f), &GenericParticleEmitter::GetVelocityRandomizeMax, &GenericParticleEmitter::SetVelocityRandomizeMax),
	_positionRandomizeMin("Position Randomize Min", Vector3(-0.5f), &GenericParticleEmitter::GetPositionRandomizeMin, &GenericParticleEmitter::SetPositionRandomizeMin),
	_positionRandomizeMax("Position Randomize Max", Vector3(0.5f), &GenericParticleEmitter::GetPositionRandomizeMax, &GenericParticleEmitter::SetPositionRandomizeMax),
	_startSize("Start Size Range", Vector2(0.5f, 1.5f), &GenericParticleEmitter::GetStartSize, &GenericParticleEmitter::SetStartSize),
	_endSize("End Size Range", Vector2(1.5f, 2.5f), &GenericParticleEmitter::GetEndSize, &GenericParticleEmitter::SetEndSize),
	_lifeSpan("Lifespan Range", Vector2(2.0f, 4.0f), &GenericParticleEmitter::GetLifeSpan, &GenericParticleEmitter::SetLifeSpan)
	{
		Initialize();
	}
	
	GenericParticleEmitter::GenericParticleEmitter(const GenericParticleEmitter *emitter) :
	ParticleEmitter(emitter),
	_startColor("Start Color", emitter->GetStartColor(), &GenericParticleEmitter::GetStartColor, &GenericParticleEmitter::SetStartColor),
	_endColor("End Color", emitter->GetEndColor(), &GenericParticleEmitter::GetEndColor, &GenericParticleEmitter::SetEndColor),
	_gravity("Gravity", emitter->GetGravity(), &GenericParticleEmitter::GetGravity, &GenericParticleEmitter::SetGravity),
	_velocity("Velocity", emitter->GetVelocity(), &GenericParticleEmitter::GetVelocity, &GenericParticleEmitter::SetVelocity),
	_velocityRandomizeMin("Velocity Randomize Min", emitter->GetVelocityRandomizeMin(), &GenericParticleEmitter::GetVelocityRandomizeMin, &GenericParticleEmitter::SetVelocityRandomizeMin),
	_velocityRandomizeMax("Velocity Randomize Max", emitter->GetVelocityRandomizeMax(), &GenericParticleEmitter::GetVelocityRandomizeMax, &GenericParticleEmitter::SetVelocityRandomizeMax),
	_positionRandomizeMin("Position Randomize Min", emitter->GetPositionRandomizeMin(), &GenericParticleEmitter::GetPositionRandomizeMin, &GenericParticleEmitter::SetPositionRandomizeMin),
	_positionRandomizeMax("Position Randomize Max", emitter->GetPositionRandomizeMax(), &GenericParticleEmitter::GetPositionRandomizeMax, &GenericParticleEmitter::SetPositionRandomizeMax),
	_startSize("Start Size Range", emitter->GetStartSize(), &GenericParticleEmitter::GetStartSize, &GenericParticleEmitter::SetStartSize),
	_endSize("End Size Range", emitter->GetEndSize(), &GenericParticleEmitter::GetEndSize, &GenericParticleEmitter::SetEndSize),
	_lifeSpan("Lifespan Range", emitter->GetLifeSpan(), &GenericParticleEmitter::GetLifeSpan, &GenericParticleEmitter::SetLifeSpan)
	{
		Initialize();
	}
	
	GenericParticleEmitter::GenericParticleEmitter(RN::Deserializer *deserializer) :
	ParticleEmitter(deserializer),
	_startColor("Start Color", Color(), &GenericParticleEmitter::GetStartColor, &GenericParticleEmitter::SetStartColor),
	_endColor("End Color", Color(1.0f, 1.0f, 1.0f, 0.0f), &GenericParticleEmitter::GetEndColor, &GenericParticleEmitter::SetEndColor),
	_gravity("Gravity", Vector3(0.0f, -0.1f, 0.0f), &GenericParticleEmitter::GetGravity, &GenericParticleEmitter::SetGravity),
	_velocity("Velocity", Vector3(0.0f, 0.5f, 0.0f), &GenericParticleEmitter::GetVelocity, &GenericParticleEmitter::SetVelocity),
	_velocityRandomizeMin("Velocity Randomize Min", Vector3(-0.5f), &GenericParticleEmitter::GetVelocityRandomizeMin, &GenericParticleEmitter::SetVelocityRandomizeMin),
	_velocityRandomizeMax("Velocity Randomize Max", Vector3(0.5f), &GenericParticleEmitter::GetVelocityRandomizeMax, &GenericParticleEmitter::SetVelocityRandomizeMax),
	_positionRandomizeMin("Position Randomize Min", Vector3(-0.5f), &GenericParticleEmitter::GetPositionRandomizeMin, &GenericParticleEmitter::SetPositionRandomizeMin),
	_positionRandomizeMax("Position Randomize Max", Vector3(0.5f), &GenericParticleEmitter::GetPositionRandomizeMax, &GenericParticleEmitter::SetPositionRandomizeMax),
	_startSize("Start Size Range", Vector2(0.5f, 1.5f), &GenericParticleEmitter::GetStartSize, &GenericParticleEmitter::SetStartSize),
	_endSize("End Size Range", Vector2(1.5f, 2.5f), &GenericParticleEmitter::GetEndSize, &GenericParticleEmitter::SetEndSize),
	_lifeSpan("Lifespan Range", Vector2(2.0f, 4.0f), &GenericParticleEmitter::GetLifeSpan, &GenericParticleEmitter::SetLifeSpan)
	{
		Initialize();
		
		_lifeSpan = deserializer->DecodeVector2();
		_startColor = deserializer->DecodeColor();
		_endColor = deserializer->DecodeColor();
		_startSize = deserializer->DecodeVector2();
		_endSize = deserializer->DecodeVector2();
		_gravity = deserializer->DecodeVector3();
		_velocity = deserializer->DecodeVector3();
		_velocityRandomizeMin = deserializer->DecodeVector3();
		_velocityRandomizeMax = deserializer->DecodeVector3();
		_positionRandomizeMin = deserializer->DecodeVector3();
		_positionRandomizeMax = deserializer->DecodeVector3();
	}
	
	void GenericParticleEmitter::Serialize(RN::Serializer *serializer)
	{
		ParticleEmitter::Serialize(serializer);
		serializer->EncodeVector2(_lifeSpan);
		serializer->EncodeColor(_startColor);
		serializer->EncodeColor(_endColor);
		serializer->EncodeVector2(_startSize);
		serializer->EncodeVector2(_endSize);
		serializer->EncodeVector3(_gravity);
		serializer->EncodeVector3(_velocity);
		serializer->EncodeVector3(_velocityRandomizeMin);
		serializer->EncodeVector3(_velocityRandomizeMax);
		serializer->EncodeVector3(_positionRandomizeMin);
		serializer->EncodeVector3(_positionRandomizeMax);
	}
	
	void GenericParticleEmitter::Initialize()
	{
		AddObservables({&_lifeSpan, &_startColor, &_endColor, &_startSize, &_endSize, &_gravity, &_velocity, &_velocityRandomizeMin, &_velocityRandomizeMax, &_positionRandomizeMin, &_positionRandomizeMax});
	}
	
	RN::Particle *GenericParticleEmitter::CreateParticle()
	{
		GenericParticle *particle = new GenericParticle();
		particle->position = _rng->RandomVector3Range(_positionRandomizeMin, _positionRandomizeMax);
		
		float lifespan = _rng->RandomFloatRange(_lifeSpan->x, _lifeSpan->y);
		particle->lifespan = lifespan;
		
		particle->gravity = _gravity;
		particle->velocity = _velocity + _rng->RandomVector3Range(_velocityRandomizeMin, _velocityRandomizeMax);
		
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
		
		particle->sizeInterpolator.SetStartValue(Vector2(_rng->RandomFloatRange(_startSize->x, _startSize->y)) * sizeScale);
		particle->sizeInterpolator.SetEndValue(Vector2(_rng->RandomFloatRange(_endSize->x, _endSize->y)) * sizeScale);
		particle->sizeInterpolator.SetDuration(lifespan);
		
		particle->colorInterpolator.SetStartValue(_startColor);
		particle->colorInterpolator.SetEndValue(_endColor);
		particle->colorInterpolator.SetDuration(lifespan);
		
		particle->Update(0.0f);
		
		return particle;
	}
}
