//
//  RNSceneNodeAttachment.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SCENENODEATTACHMENT_H__
#define __RAYNE_SCENENODEATTACHMENT_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class SceneNodeAttachment : public Object
	{
	public:
		friend class SceneNode;
		RNAPI ~SceneNodeAttachment();

		RNAPI SceneNode *GetParent() const;
		RNAPI virtual void Update(float delta);

	protected:
		RNAPI SceneNodeAttachment();

	private:
		SceneNode *_node;

		__RNDeclareMetaInternal(SceneNodeAttachment)
	};
}


#endif /* __RAYNE_SCENENODEATTACHMENT_H__ */
