//
//  RNOctreeSceneManager.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCTREESCENEMANAGER_H__
#define __RAYNE_OCTREESCENEMANAGER_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNCamera.h"
#include "RNEntity.h"
#include "RNRenderer.h"
#include "RNHit.h"
#include "RNSceneManager.h"

namespace RN
{
	class OctreeNode
	{
	public:
		OctreeNode(const Vector3 &min, const Vector3 &max)
		{
			
		}
		
		~OctreeNode()
		{
			
		}
		
		void Insert(const Vector3 &min, const Vector3 &max, const SceneNode *node)
		{
			
		}
		
		void Remove(const Vector3 &min, const Vector3 &max, const SceneNode *node)
		{
			
		}
		
		void Get(const Vector3 &min, const Vector3 &max) const
		{
			
		}
		
	private:
		Vector3 _min;
		Vector3 _max;
		
		std::vector<SceneNode*> _nodes;
		OctreeNode *_children[8];
	};
	
	class Octree
	{
	public:
		Octree()
		{
			
		}
		
		~Octree()
		{
			
		}
		
		void Insert(const Vector3 &min, const Vector3 &max, const SceneNode *node)
		{
			
		}
		
		void Remove(const Vector3 &min, const Vector3 &max, const SceneNode *node)
		{
			
		}
		
		void Get(const Vector3 &min, const Vector3 &max) const
		{
			
		}
		
	private:
		OctreeNode *_root;
	};
	
	class OctreeSceneManager : public SceneManager
	{
	public:
		RNAPI OctreeSceneManager();
		RNAPI ~OctreeSceneManager() override;
		
		RNAPI void AddSceneNode(SceneNode *node) override;
		RNAPI void RemoveSceneNode(SceneNode *node) override;
		RNAPI void UpdateSceneNode(SceneNode *node, SceneNode::ChangeSet changes) override;
		
		RNAPI void RenderScene(Camera *camera) override;
		
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, uint32 mask = 0xffff, Hit::HitMode mode = Hit::HitMode::IgnoreNone) override;
		
	private:
		void RenderSceneNode(Camera *camera, SceneNode *node);
		
		std::vector<SceneNode *> _nodes;
		
		Octree _octree;
		
		RNDeclareMetaWithTraits(OctreeSceneManager, SceneManager, MetaClassTraitCronstructable);
	};
}

#endif /* __RAYNE_OCTREESCENEMANAGER_H__ */
