//
//  RNSceneManager.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEMANAGER_H__
#define __RAYNE_SCENEMANAGER_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNCamera.h"
#include "RNEntity.h"
#include "RNRenderer.h"
#include "RNHit.h"

namespace RN
{
	class SceneManager : public Object
	{
	public:
		virtual void AddSceneNode(SceneNode *node) = 0;
		virtual void RemoveSceneNode(SceneNode *node) = 0;
		virtual void UpdateSceneNode(SceneNode *node) = 0;
		
		virtual void RenderScene(Camera *camera) = 0;
		virtual Hit CastRay(const Vector3 &position, const Vector3 &direction, uint32 mask = 0x00ff, Hit::HitMode mode = Hit::HitMode::IgnoreNone) = 0;
		
	protected:
		SceneManager();
		virtual ~SceneManager();
		
		Renderer *_renderer;
		
		RNDefineMeta(SceneManager, Object)
	};
	
	class GenericSceneManager : public SceneManager
	{
	public:
		GenericSceneManager();
		~GenericSceneManager() override;
		
		void AddSceneNode(SceneNode *node) override;
		void RemoveSceneNode(SceneNode *node) override;
		void UpdateSceneNode(SceneNode *node) override;
		
		void RenderScene(Camera *camera) override;
		
		Hit CastRay(const Vector3 &position, const Vector3 &direction, uint32 mask = 0xffff, Hit::HitMode mode = Hit::HitMode::IgnoreNone) override;
		
	private:
		void RenderSceneNode(Camera *camera, SceneNode *node);
		
		std::vector<SceneNode *> _nodes;
		std::unordered_set<SceneNode *> _rootNodes;
		
		RNDefineMetaWithTraits(GenericSceneManager, SceneManager, MetaClassTraitCronstructable);
	};
}

#endif /* __RAYNE_SCENEMANAGER_H__ */
