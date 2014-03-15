//
//  RNWorld.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLD_H__
#define __RAYNE_WORLD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNThread.h"
#include "RNRenderer.h"
#include "RNSceneNode.h"
#include "RNCamera.h"
#include "RNWorldAttachment.h"
#include "RNSceneManager.h"

namespace RN
{
	class Kernel;
	class WorldCoordinator;
	
	class World : public Object
	{
	public:
		friend class SceneNode;
		friend class Kernel;
		friend class WorldCoordinator;
		
		enum class Mode
		{
			Play,
			Edit
		};
		
		RNAPI World(SceneManager *sceneManager);
		RNAPI World(const std::string& sceneManager);
		RNAPI ~World() override;
		
		RNAPI void SetReleaseSceneNodesOnDestruction(bool releaseSceneNodes);
		RNAPI void SetMode(Mode mode);
		
		RNAPI void AddAttachment(WorldAttachment *attachment);
		RNAPI void RemoveAttachment(WorldAttachment *attachment);
		
		RNAPI void AddSceneNode(SceneNode *node);
		RNAPI void RemoveSceneNode(SceneNode *node);
		RNAPI void DropSceneNodes();
		RNAPI void ApplyNodes();
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual void UpdateEditMode(float delta);
		RNAPI virtual void DidUpdateToFrame(FrameID frame);
		
		RNAPI virtual void LoadOnThread(Thread *thread, Deserializer *deserializer);
		RNAPI virtual void FinishLoading(Deserializer *deserializer);
		RNAPI virtual bool SupportsBackgroundLoading() const;
		
		RNAPI virtual void SaveOnThread(Thread *thread, Serializer *serializer);
		RNAPI virtual void FinishSaving(Serializer *serializer);
		RNAPI virtual bool SupportsBackgroundSaving() const;
		
		RNAPI SceneManager *GetSceneManager() const { return _sceneManager; }
		RNAPI Array *GetSceneNodes();
		RNAPI Mode GetMode() const { return _mode; }
		
		template<class T>
		T *GetSceneNodeWithTag(Tag tag)
		{
			return static_cast<T *>(__GetSceneNodeWithTag(tag, T::MetaClass()));
		}
		
		template<class T>
		Array *GetSceneNodesWithTag(Tag tag)
		{
			return __GetSceneNodesWithTag(tag, T::MetaClass());
		}
		
		RNAPI static World *GetActiveWorld();
		
	private:		
		static class SceneManager *SceneManagerWithName(const std::string& name);
		
		RNAPI SceneNode *__GetSceneNodeWithTag(Tag tag, MetaClassBase *meta);
		RNAPI Array *__GetSceneNodesWithTag(Tag tag, MetaClassBase *meta);
		
		void StepWorld(FrameID frame, float delta);
		void StepWorldEditMode(FrameID frame, float delta);
		void RenderWorld(Renderer *renderer);
		bool __RemoveSceneNode(SceneNode *node);
		
		void SceneNodeWillRender(SceneNode *node);
		
		void SceneNodeWillUpdate(SceneNode *node, SceneNode::ChangeSet changeSet);
		void SceneNodeDidUpdate(SceneNode *node, SceneNode::ChangeSet changeSet);
		void DropSceneNode(SceneNode *node);
		
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
		
		Mode _mode;
		Kernel *_kernel;
		Array _attachments;
		
		bool _isDroppingSceneNodes;
		bool _releaseSceneNodesOnDestructor;
		bool _requiresResort;
		bool _requiresCameraSort;
		
		RecursiveSpinLock _nodeLock;
		
		std::vector<SceneNode *> _addedNodes;
		std::vector<SceneNode *> _nodes;
		std::vector<SceneNode *> _staticNodes;
		
		std::vector<Camera *> _cameras;
		std::unordered_map<Tag, std::unordered_set<SceneNode *>> _tagTable;
		
		SceneManager  *_sceneManager;
		MetaClassBase *_cameraClass;
		
		RNDeclareMeta(World)
	};
}

#endif /* __RAYNE_WORLD_H__ */
