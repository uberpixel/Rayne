//
//  RNInstancingNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingNode.h"
#include "RNRenderer.h"

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
	
	
	void InstancingNode::ChildDidUpdate(SceneNode *child)
	{
		MarkChildDirty(child);
	}
	
	void InstancingNode::DidAddChild(SceneNode *child)
	{
		MarkChildDirty(child);
	}
	
	void InstancingNode::WillRemoveChild(SceneNode *child)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);			
			entity->_ignoreDrawing = false;
		}
	}
	
	void InstancingNode::MarkChildDirty(SceneNode *child)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);
			Model *entityModel = entity->Model();
			
			if(entityModel == _model)
				_dirty = true;
		}
	}
	
	
	
	void InstancingNode::SetModel(Model *model)
	{
		if(_model)
			_model->Release();
		
		_model = model ? model->Retain() : 0;
		_dirty = true;
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
			object.ivbo = i->vbo;
			object.count = i->count;
			object.offset = i->offset;
			object.transform = (Matrix *)&WorldTransform();
			
			renderer->RenderObject(object);
		}
	}
	
	
	
	void InstancingNode::GenerateDataForMesh(const Array<Entity *>& entities, Mesh *mesh, Material *material)
	{		
		size_t count = entities.Count();
		
		GLuint vbo;
		Matrix *matrices = new Matrix[count * 2];
		
		for(machine_uint i=0; i<count; i++)
		{
			matrices[(int)i] = entities[(int)i]->WorldTransform();
			matrices[(int)i + count] = entities[(int)i]->WorldTransform().Inverse();
		}
		
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, count * 2 * sizeof(Matrix), matrices, GL_STATIC_DRAW);
		
		InstancedMesh imesh;
		imesh.mesh = mesh;
		imesh.material = material;
		imesh.vbo = vbo;
		imesh.count = static_cast<uint32>(count);
		imesh.offset = static_cast<uint32>(count);
		
		_data.push_back(imesh);
	}
	
	void InstancingNode::RecalculateData()
	{
		Array<Entity *> entities;
		
		for(auto i=_data.begin(); i!=_data.end(); i++)
		{
			glDeleteBuffers(1, &i->vbo);
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
					entity->_ignoreDrawing = true;
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
