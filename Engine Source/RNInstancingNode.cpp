//
//  RNInstancingNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingNode.h"
#include "RNRenderer.h"
#include "RNNumber.h"
#include "RNThreadPool.h"

#define kRNInstancingNodeAssociatedIndexKey "kRNInstancingNodeAssociatedIndexKey"

namespace RN
{
	InstancingNode::InstancingNode()
	{
		_entityClass = Entity::MetaClass();
		_dirty = 0;
		_model = 0;
	}
	
	InstancingNode::InstancingNode(Model *model)
	{
		_entityClass = Entity::MetaClass();
		_dirty = 0;
		_model = 0;
		
		SetModel(model);
	}
	
	InstancingNode::~InstancingNode()
	{
		for(InstancedMesh& mesh : _data)
		{
			glDeleteTextures(1, &mesh.texture);
			glDeleteBuffers(1, &mesh.buffer);
		}
	}
	
	
	void InstancingNode::SetModel(Model *model)
	{
		if(_model)
			_model->Release();
		
		_model = model ? model->Retain() : 0;
		_dirty = true;
	}
	
	
	
	void InstancingNode::ChildDidUpdate(SceneNode *child)
	{
		MarkChildDirty(child, true);
	}
	
	void InstancingNode::DidAddChild(SceneNode *child)
	{
		MarkChildDirty(child, false);
	}
	
	void InstancingNode::WillRemoveChild(SceneNode *child)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);			
			entity->_ignoreDrawing = false;
			entity->RemoveAssociatedOject(kRNInstancingNodeAssociatedIndexKey);
			
			_dirty = true;
		}
	}
	
	void InstancingNode::MarkChildDirty(SceneNode *child, bool canRecover)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);
			Model *entityModel = entity->GetModel();
			
			if(entityModel == _model)
			{
				if(canRecover)
				{
					if(_dirty)
						return;
					
					Number *index = (Number *)entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey);
					
					size_t size = _data.size();
					
					for(size_t i=0; i<size; i++)
						UpdateDataForMesh(entity, _data[i], index->GetUint32Value());
				}
				else
				{
					_dirty = true;
				}
			}
		}
	}
	
	
	
	bool InstancingNode::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void InstancingNode::Render(Renderer *renderer, Camera *camera)
	{
		if(_dirty)
			RecalculateData();
		
		for(auto i=_data.begin(); i!=_data.end(); i++)
		{
			RenderingObject object(RenderingObject::Type::Instanced);
			
			object.mesh = i->mesh;
			object.material = i->material;
			object.instancingData = i->texture;
			object.count = i->count;
			object.rotation = (Quaternion*)&GetWorldRotation();
			object.transform = (Matrix *)&GetWorldTransform();
			
			renderer->RenderObject(object);
		}
		
		SceneNode::Render(renderer, camera);
	}
	
	
	
	void InstancingNode::GenerateDataForMesh(const std::vector<Entity *>& entities, Mesh *mesh, Material *material)
	{		
		size_t count = entities.size();
		
		GLuint texture;
		GLuint buffer;
		
		Matrix *matrices = new Matrix[count * 2];
		
		for(size_t i=0; i<count; i++)
		{
			matrices[(int)((i * 2) + 0)] = entities[i]->GetWorldTransform();
			matrices[(int)((i * 2) + 1)] = entities[i]->GetWorldTransform().GetInverse();
		}
		
		glGenTextures(1, &texture);
		glGenBuffers(1, &buffer);
		
		RN_CHECKOPENGL_AGGRESSIVE();
		
		glBindTexture(GL_TEXTURE_BUFFER, texture); RN_CHECKOPENGL_AGGRESSIVE();
		glBindBuffer(GL_TEXTURE_BUFFER, buffer); RN_CHECKOPENGL_AGGRESSIVE();
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffer); RN_CHECKOPENGL_AGGRESSIVE();
		
		size_t size = count * 2 * sizeof(Matrix);
		glBufferData(GL_TEXTURE_BUFFER, static_cast<GLsizei>(size), matrices, GL_DYNAMIC_DRAW); RN_CHECKOPENGL_AGGRESSIVE();
		
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
		
		RN_CHECKOPENGL();
		
		delete [] matrices;
		
		InstancedMesh imesh;
		imesh.mesh = mesh;
		imesh.material = material;
		imesh.texture = texture;
		imesh.buffer = buffer;
		imesh.count = static_cast<uint32>(count);
		
		_data.push_back(std::move(imesh));
	}
	
	void InstancingNode::UpdateDataForMesh(Entity *entity, const InstancedMesh& mesh, uint32 index)
	{
		Matrix matrices[2];
		matrices[0] = entity->GetWorldTransform();
		matrices[1] = entity->GetWorldTransform().GetInverse();
		
		size_t offset = index * (sizeof(Matrix) * 2);
		
		glBindBuffer(GL_TEXTURE_BUFFER, mesh.buffer);
		glBufferSubData(GL_TEXTURE_BUFFER, offset, 2 * sizeof(Matrix), matrices);
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
	}
	
	
	void InstancingNode::RecalculateData()
	{
		std::vector<Entity *> entities;
		
		for(InstancedMesh& mesh : _data)
		{
			glDeleteTextures(1, &mesh.texture);
			glDeleteBuffers(1, &mesh.buffer);
		}
		
		size_t childs = GetChildCount();
		entities.reserve(childs);
		
		for(size_t i=0; i<childs; i++)
		{
			SceneNode *node = GetChildAtIndex(i);
			
			if(node->IsKindOfClass(_entityClass))
			{
				Entity *entity = static_cast<Entity *>(node);
				Model *entityModel = entity->GetModel();
				
				entity->_ignoreDrawing = false;
				
				if(entityModel == _model)
				{
					uint32 index = static_cast<uint32>(entities.size());
					
					entity->_ignoreDrawing = true;
					entity->SetAssociatedObject(kRNInstancingNodeAssociatedIndexKey, Number::WithUint32(index), Object::MemoryPolicy::Retain);
					
					entities.push_back(entity);
				}
			}
		}
		
		if(entities.size() > 0)
		{
			uint32 count = _model->GetMeshCount(0);
			for(uint32 i=0; i<count; i++)
			{
				Mesh *mesh = _model->GetMeshAtIndex(0, i);
				Material *material = _model->GetMaterialAtIndex(0, i);
				
				GenerateDataForMesh(entities, mesh, material);
			}
		}
		
		_dirty = false;
	}
}
