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

namespace RN
{
	class SceneManager : public Object
	{
	public:
		virtual void AddSceneNode(SceneNode *node) = 0;
		virtual void RemoveSceneNode(SceneNode *node) = 0;
		virtual void UpdateSceneNode(SceneNode *node) = 0;
		
		virtual void RenderScene(Camera *camera) = 0;
		
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
		virtual ~GenericSceneManager();
		
		virtual void AddSceneNode(SceneNode *node);
		virtual void RemoveSceneNode(SceneNode *node);
		virtual void UpdateSceneNode(SceneNode *node);
		
		virtual void RenderScene(Camera *camera);
		
	private:
		void RenderSceneNode(Camera *camera, SceneNode *node);
		
		std::unordered_set<SceneNode *> _nodes;
		class MetaClass *_entityClass;
		class MetaClass *_lightClass;
		
		RNDefineMetaWithTraits(GenericSceneManager, SceneManager, MetaClassTraitCreatable);
	};
}

#endif /* __RAYNE_SCENEMANAGER_H__ */
