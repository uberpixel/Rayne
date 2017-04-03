//
//  RNOpenALListener.h
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENALLISTENER_H_
#define __RAYNE_OPENALLISTENER_H_

#include "RNOpenAL.h"

namespace RN
{
	class OpenALListener : public SceneNodeAttachment
	{
	public:
		friend class OpenALWorld;
			
		OALAPI OpenALListener();
		OALAPI ~OpenALListener() override;
			
		OALAPI void Update(float delta) override;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//void DidAddToParent() override;
		//void WillRemoveFromParent() override;
			
		void ReInsertIntoWorld();
		virtual void InsertIntoWorld(OpenALWorld *world);
		virtual void RemoveFromWorld();
			
	private:
		OpenALWorld *_owner;
			
		Vector3 _oldPosition;
			
		RNDeclareMetaAPI(OpenALListener, OALAPI)
	};
}

#endif /* defined(__RAYNE_OPENALLISTENER_H_) */
