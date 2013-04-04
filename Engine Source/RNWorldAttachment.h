//
//  RNWorldAttachment.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLDATTACHMENT_H__
#define __RAYNE_WORLDATTACHMENT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTransform.h"
#include "RNCamera.h"

namespace RN
{
	class WorldAttachment : public Object
	{
	public:
		RNAPI virtual void UpdateTransform(Transform *transform, float delta);
		RNAPI virtual void StepWorld(float delta);
		RNAPI virtual void TransformsUpdated();
		RNAPI virtual void BeginCamera(Camera *camera);
		RNAPI virtual void WillFinishCamera(Camera *camera);
		
		RNAPI virtual void DidAddTransform(Transform *transform);
		RNAPI virtual void WillRemoveTransform(Transform *transform);
		
		RNDefineConstructorlessMeta(WorldAttachment, Object)
	};
}

#endif /* __RAYNE_WORLDATTACHMENT_H__ */
