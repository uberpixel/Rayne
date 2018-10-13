//
//  RNSceneAttachment.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEATTACHMENT_H__
#define __RAYNE_SCENEATTACHMENT_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class Scene;
	class SceneAttachment : public Object
	{
	public:
		friend class SceneManager;
		friend class Scene;

		RNAPI ~SceneAttachment();

		RNAPI Scene *GetParent() const;

	protected:
		RNAPI SceneAttachment();
		RNAPI virtual void Update(float delta);

	private:
		Scene *_scene;

		__RNDeclareMetaInternal(SceneAttachment)
	};
}


#endif /* __RAYNE_SCENEATTACHMENT_H__ */
