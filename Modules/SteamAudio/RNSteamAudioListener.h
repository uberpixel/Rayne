//
//  RNSteamAudioListener.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOLISTENER_H_
#define __RAYNE_STEAMAUDIOLISTENER_H_

#include "RNSteamAudio.h"

namespace RN
{
	class SteamAudioListener : public SceneNodeAttachment
	{
	public:
		friend class SteamAudioWorld;
			
		SAAPI SteamAudioListener();
		SAAPI ~SteamAudioListener() override;
			
		SAAPI void Update(float delta) override;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//void DidAddToParent() override;
		//void WillRemoveFromParent() override;
			
		void ReInsertIntoWorld();
		virtual void InsertIntoWorld(SteamAudioWorld *world);
		virtual void RemoveFromWorld();
			
	private:
		SteamAudioWorld *_owner;
			
		Vector3 _oldPosition;
			
		RNDeclareMetaAPI(SteamAudioListener, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOLISTENER_H_) */
