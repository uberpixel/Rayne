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
	struct SceneSortSpan
	{
		size_t first = 0;
		size_t end = 0;

		void Include(size_t slot)
		{
			if(end == 0) first = slot;
			end = slot + 1;
		}
	};

	template<class T>
	struct SceneSortKey
	{
		T *node;
		double distance;
		int32 priority;
	};

	// Cache keys once and reuse the scene's scratch capacity between cameras.
	// Transparent-only sorting preserves the positions of every other priority.
	template<bool FrontToBack, class T, class PriorityProject, class DistanceProject>
	inline void SortSceneNodes(T **values, size_t count, std::vector<SceneSortKey<T>> &scratch, PriorityProject getRenderPriority, DistanceProject getSquaredDistance, int skyThreshold, int transparentPriority, bool sortTransparent = false)
	{
		if(count < 2) return;

		scratch.clear();
		if constexpr(FrontToBack) scratch.reserve(count);

		for(size_t i = 0; i < count; ++i)
		{
			T *node = values[i];
			const int32 renderPriority = getRenderPriority(node);
			if(!FrontToBack && renderPriority != transparentPriority) continue;
			const bool backToFront = !FrontToBack || (sortTransparent && renderPriority == transparentPriority);
			const double distance = (backToFront || renderPriority < skyThreshold) ? getSquaredDistance(node) : 0;
			// A negative distance lets both directions share one ascending comparator.
			scratch.push_back({node, backToFront ? -distance : distance, renderPriority});
		}

		if(scratch.size() < 2) return;

		std::sort(scratch.begin(), scratch.end(), [](const SceneSortKey<T> &a, const SceneSortKey<T> &b) {
			return a.priority == b.priority ? a.distance < b.distance : a.priority < b.priority;
		});

		size_t scratchIndex = 0;
		for(size_t i = 0; i < count; ++i)
		{
			// Priorities remain unchanged while collecting, sorting and scattering.
			if(FrontToBack || getRenderPriority(values[i]) == transparentPriority)
			{
				values[i] = scratch[scratchIndex++].node;
			}
		}
	}

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

		virtual bool AllowsParallelNodeUpdates() const { return true; }

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
		std::vector<SceneSortKey<SceneNode>> _sceneSortScratch;
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
