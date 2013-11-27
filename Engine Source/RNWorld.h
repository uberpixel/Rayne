//
//  RNWorld.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLD_H__
#define __RAYNE_WORLD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNRenderer.h"
#include "RNSceneNode.h"
#include "RNCamera.h"
#include "RNWorldAttachment.h"
#include "RNSceneManager.h"

namespace RN
{
	class Kernel;

	class World : public NonConstructingSingleton<World>
	{
	public:
		friend class SceneNode;
		friend class Kernel;
		
		RNAPI World(SceneManager *sceneManager);
		RNAPI World(const std::string& sceneManager);
		RNAPI ~World() override;
		
		RNAPI void SetReleaseSceneNodesOnDestruction(bool releaseSceneNodes);
		
		RNAPI void AddAttachment(WorldAttachment *attachment);
		RNAPI void RemoveAttachment(WorldAttachment *attachment);
		
		RNAPI void AddSceneNode(SceneNode *node);
		RNAPI void RemoveSceneNode(SceneNode *node);
		RNAPI void DropSceneNodes();
		
		RNAPI void StepWorld(FrameID frame, float delta);
		RNAPI void RenderWorld(Renderer *renderer);
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual void UpdatedToFrame(FrameID frame);
		RNAPI virtual void WillRenderSceneNode(SceneNode *node);
		RNAPI virtual void Reset();
		
		RNAPI SceneManager *GetSceneManager() const { return _sceneManager; }
		
	private:		
		static class SceneManager *SceneManagerWithName(const std::string& name);
		
		void SceneNodeWillRender(SceneNode *node);
		
		void SceneNodeDidUpdate(SceneNode *node, uint32 changeSet);
		void ApplyNodes();
		
		void SortNodes();
		void SortCameras();
		
		template<class F, class ...Args>
		void RunWorldAttachement(F function, Args&&... args)
		{
			size_t count = _attachments.GetCount();
			
			for(size_t i = 0; i < count; i ++)
			{
				WorldAttachment *attachment = static_cast<WorldAttachment *>(_attachments[i]);
				(attachment->*function)(std::forward<Args>(args)...);
			}
		}
		
		Kernel *_kernel;
		Array _attachments;
		
		bool _isDroppingSceneNodes;
		bool _releaseSceneNodesOnDestructor;
		bool _requiresResort;
		bool _requiresCameraSort;
		
		SpinLock _nodeLock;
		
		std::vector<SceneNode *> _addedNodes;
		std::vector<SceneNode *> _nodes;
		
		std::vector<Camera *> _cameras;
		
		SceneManager  *_sceneManager;
		MetaClassBase *_cameraClass;
	};
}

#endif /* __RAYNE_WORLD_H__ */
