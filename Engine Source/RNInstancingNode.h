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
		~InstancingNode() override;
		
		RNAPI void SetModel(Model *model);
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		
	protected:
		RNAPI void ChildDidUpdate(SceneNode *child) override;
		RNAPI void DidAddChild(SceneNode *child) override;
		RNAPI void WillRemoveChild(SceneNode *child) override;
		
	private:
		struct InstancedMesh
		{
			~InstancedMesh()
			{
				glDeleteTextures(1, &texture);
				glDeleteBuffers(1, &buffer);
			}
			
			Mesh *mesh;
			Material *material;
			
			GLuint texture;
			GLuint buffer;
			
			uint32 count;
		};
		
		void MarkChildDirty(SceneNode *child, bool canRecover);
		void GenerateDataForMesh(const std::vector<Entity *>& entities, Mesh *mesh, Material *material);
		void UpdateDataForMesh(Entity *entity, const InstancedMesh& mesh, uint32 index);
		void RecalculateData();
		
		bool _dirty;
		Model *_model;
		std::vector<InstancedMesh> _data;
		
		class MetaClass *_entityClass;
	};
}

#endif /* __RAYNE_INSTANCINGNODE_H__ */
