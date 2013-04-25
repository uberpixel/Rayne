//
//  RNInstancingNode.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INSTANCINGNODE_H__
#define __RAYNE_INSTANCINGNODE_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNEntity.h"

namespace RN
{
	class InstancingNode : public SceneNode
	{
	public:
		InstancingNode();
		InstancingNode(Model *model);
		
		RNAPI void SetModel(Model *model);
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		RNAPI virtual void Render(Renderer *renderer, Camera *camera);
		
	protected:
		RNAPI virtual void ChildDidUpdate(SceneNode *child);
		RNAPI virtual void DidAddChild(SceneNode *child);
		RNAPI virtual void WillRemoveChild(SceneNode *child);
		
	private:
		void MarkChildDirty(SceneNode *child);
		void GenerateDataForMesh(const Array<Entity *>& entities, Mesh *mesh, Material *material);
		void RecalculateData();
		
		struct InstancedMesh
		{
			Mesh *mesh;
			Material *material;
			
			GLuint vbo;
			
			uint32 count;
			uint32 offset;
		};
		
		bool _dirty;
		Model *_model;
		std::vector<InstancedMesh> _data;
		
		class MetaClass *_entityClass;
	};
}

#endif /* __RAYNE_INSTANCINGNODE_H__ */
