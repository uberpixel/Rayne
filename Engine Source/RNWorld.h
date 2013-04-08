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
#include "RNTransform.h"
#include "RNCamera.h"
#include "RNWorldAttachment.h"

namespace RN
{
	class Kernel;
	class Transform;
	
	class World : public NonConstructingSingleton<World>
	{
	friend class Transform;
	friend class Kernel;
	public:
		RNAPI World();
		RNAPI virtual ~World();
		
		RNAPI void AddTransform(Transform *transform);
		RNAPI void RemoveTransform(Transform *transform);
		
		RNAPI void AddAttachment(WorldAttachment *attachment);
		RNAPI void RemoveAttachment(WorldAttachment *attachment);
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual void TransformsUpdated();
		
	private:
		void StepWorld(FrameID frame, float delta);
		void VisitTransform(Camera *camera, Transform *transform);
		bool SupportsTransform(Transform *transform);
		void ApplyTransformUpdates();
		
		Kernel *_kernel;
		
		Array<WorldAttachment> _attachments;
		std::unordered_set<Transform *> _transforms;
		std::deque<Transform *> _addedTransforms;
		std::vector<Camera *> _cameras;
		
		Renderer *_renderer;
		
		MetaClass *_cameraClass;
		MetaClass *_entityClass;
		MetaClass *_lightClass;
	};
}

#endif /* __RAYNE_WORLD_H__ */
