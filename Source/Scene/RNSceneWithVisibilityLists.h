//
//  RNSceneWithVisibilityLists.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEWITHVISIBILITYLISTS_H__
#define __RAYNE_SCENEWITHVISIBILITYLISTS_H__

#include "RNScene.h"

namespace RN
{
	class SceneWithVisibilityLists : public Scene
	{
	public:
		class Volume : public Object
		{
		public:
			RNAPI virtual bool ContainsPosition(const RN::Vector3 &cameraPosition) const;
			std::vector<SceneNode *> nodes;
			
			RN::Vector3 boundsMin;
			Vector3 boundsMax;
			
			__RNDeclareMetaInternal(Volume)
		};
		
		class AxisAlignedBoxVolume : public Volume
		{
		public:
			RNAPI bool ContainsPosition(const RN::Vector3 &cameraPosition) const final;
			
			__RNDeclareMetaInternal(AxisAlignedBoxVolume)
		};
		
		RNAPI ~SceneWithVisibilityLists();
		
		RNAPI void AddVolume(Volume *volume);
		
		RNAPI void AddNode(SceneNode *node) override;
		RNAPI void RemoveNode(SceneNode *node) override;

	protected:
		RNAPI SceneWithVisibilityLists();

		RNAPI void Update(float delta) override;
		RNAPI void Render(Renderer *renderer) override;
		
		RNAPI void RenderVolumeList(int drawPriority, Renderer *renderer, Camera *camera, const Volume *volume);

		IntrusiveList<SceneNode> _nodes[3];
		IntrusiveList<Camera> _cameras;
		
		Array *_volumes;
		Volume *_defaultVolume;
		
		bool _isAddingVolume;

		__RNDeclareMetaInternal(SceneWithVisibilityLists)
	};
	
	class SceneWithVisibilityListsInfo : public SceneInfo
	{
	public:
		SceneWithVisibilityListsInfo(Scene *scene);
		
		std::vector<SceneWithVisibilityLists::Volume *>volumes;
		
		__RNDeclareMetaInternal(SceneWithVisibilityListsInfo)
	};
}


#endif /* __RAYNE_SCENEWITHVISIBILITYLISTS_H__ */
