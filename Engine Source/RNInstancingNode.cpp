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
		for(auto i=_data.begin(); i!=_data.end(); i++)
		{
			GLuint texture = i->texture;
			glDeleteTextures(1, &texture);
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
			Model *entityModel = entity->Model();
			
			if(entityModel == _model)
			{
				if(canRecover)
				{
					if(_dirty)
						return;
					
					Number *tindex = (Number *)entity->AssociatedObject(kRNInstancingNodeAssociatedIndexKey);
					uint32 index = tindex->Uint32Value();
					
					size_t size = _data.size();
					for(size_t i=0; i<size; i++)
					{
						UpdateDataForMesh(entity, _data[i], index);
					}
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
		
		RenderingObject object(RenderingObject::TypeInstanced);

		for(auto i=_data.begin(); i!=_data.end(); i++)
		{
			object.mesh = i->mesh;
			object.material = i->material;
			object.texture = i->texture;
			object.count = i->count;
			object.transform = (Matrix *)&WorldTransform();
			
			renderer->RenderObject(object);
		}
	}
	
	
	
	void InstancingNode::GenerateDataForMesh(const Array<Entity *>& entities, Mesh *mesh, Material *material)
	{		
		size_t count = entities.Count();
		
		GLuint texture;
		Matrix *matrices = new Matrix[count * 2];
		
		for(machine_uint i=0; i<count; i++)
		{
			matrices[(int)i] = entities[(int)i]->WorldTransform();
			matrices[(int)i + count] = entities[(int)i]->WorldTransform().Inverse();
		}
		
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, static_cast<GLsizei>(count * 4), 2, 0, GL_RGBA, GL_FLOAT, matrices);
		
		InstancedMesh imesh;
		imesh.mesh = mesh;
		imesh.material = material;
		imesh.texture = texture;
		imesh.count = static_cast<uint32>(count);
		
		_data.push_back(imesh);
	}
	
	void InstancingNode::UpdateDataForMesh(Entity *entity, const InstancedMesh& mesh, uint32 index)
	{
		Matrix matrices[2];
		matrices[0] = entity->WorldTransform();
		matrices[1] = entity->WorldTransform().Inverse();
		
		glBindTexture(GL_TEXTURE_2D, mesh.texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, index * 4, 0, 4, 2, GL_RGBA, GL_FLOAT, matrices);
	}
	
	
	void InstancingNode::RecalculateData()
	{
		Array<Entity *> entities;
		
		for(auto i=_data.begin(); i!=_data.end(); i++)
		{
			GLuint texture = i->texture;
			glDeleteTextures(1, &texture);
		}
		
		_data.clear();
		
		machine_uint childs = Childs();
		for(machine_uint i=0; i<childs; i++)
		{
			SceneNode *node = ChildAtIndex(i);
			
			if(node->IsKindOfClass(_entityClass))
			{
				Entity *entity = static_cast<Entity *>(node);
				Model *entityModel = entity->Model();
				
				entity->_ignoreDrawing = false;
				
				if(entityModel == _model)
				{
					uint32 index = static_cast<uint32>(entities.Count());
					
					entity->_ignoreDrawing = true;
					entity->SetAssociatedObject(kRNInstancingNodeAssociatedIndexKey, Number::WithUint32(index), Object::MemoryPolicy::Retain);
					
					entities.AddObject(entity);
				}
			}
		}
		
		if(entities.Count() > 0)
		{
			uint32 count = _model->Meshes(0);
			for(uint32 i=0; i<count; i++)
			{
				Mesh *mesh = _model->MeshAtIndex(0, i);
				Material *material = _model->MaterialAtIndex(0, i);
				
				GenerateDataForMesh(entities, mesh, material);
			}
		}
		
		_dirty = false;
	}
}
