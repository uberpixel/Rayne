//
//  RNSceneBasic.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEBASIC_H__
#define __RAYNE_SCENEBASIC_H__

#include "RNOcclusionCuller.h"
#include "RNScene.h"

namespace RN
{
	class SceneBasic : public Scene
	{
	public:
		struct OcclusionCullingParameters
		{
			RNAPI OcclusionCullingParameters(uint16 textureWidth = 40, uint16 textureHeight = 40, size_t maxOccluders = 150, uint16 frameCount = 50, float jitter = 0.15f);

			uint16 textureWidth;
			uint16 textureHeight;
			size_t maxOccluders;
			uint16 frameCount;
			float jitter;
		};

		RNAPI ~SceneBasic();

		RNAPI void AddNode(SceneNode *node) override;
		RNAPI void RemoveNode(SceneNode *node) override;

	protected:
		RNAPI SceneBasic(const OcclusionCullingParameters &occlusionCullingParameters = OcclusionCullingParameters());

		RNAPI void Update(float delta) override;
		RNAPI void Render(Renderer *renderer) override;
		RNAPI void PrepareForShutdown() override;

		RNAPI void FlushAdditionQueue();
		RNAPI void FlushDeletionQueue();

		RNAPI void AddRenderNode(SceneNode *node);
		RNAPI void RemoveRenderNode(SceneNode *node);

		RNAPI void MakeDrawablesDirty();

		//Should probably be private, but this makes it easy to visualize
		OcclusionCuller *_occlusionCuller;

	private:
		IntrusiveList<SceneNode> _updateNodes[4];
		IntrusiveList<SceneNode> _renderNodes;
		// Non-owning cache in _renderNodes order, rebuilt after membership or priority changes.
		std::vector<SceneNode *> _renderNodesCache;
		bool _renderNodesCacheDirty;
		IntrusiveList<Light> _lights;
		IntrusiveList<Camera> _cameras;
		Array *_nodesToRemove;
		Array *_nodesToAdd;

		void RebuildRenderNodesCache();
		bool IsOccluderCacheCandidate(SceneNode *node) const;
		void AddCachedOccluderNode(SceneNode *node);
		void RemoveCachedOccluderNode(SceneNode *node);
		void RemoveAllNodes();

		std::vector<SceneNode *> _occluderNodes;

		OcclusionCullingParameters _occlusionCullingParameters;
		size_t _currentFrameCount;

		__RNDeclareMetaInternal(SceneBasic)
	};

	class SceneBasicInfo : public SceneInfo
	{
	public:
		SceneBasicInfo(Scene *scene);

		uint16 occludedFrameCounter;
		float occluderSize;
		float occluderDistance;
		bool isActiveOccluder;
		bool isVisibleOccluder;
		bool isCachedOccluder;

		__RNDeclareMetaInternal(SceneBasicInfo)
	};
} // namespace RN


#endif /* __RAYNE_SCENEBASIC_H__ */
