//
//  RNRecastWorld.h
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECASTWORLD_H_
#define __RAYNE_RECASTWORLD_H_

#include "RNRecast.h"

namespace RN
{
	class RecastWorld : public SceneAttachment
	{
	public:
		RCAPI static RecastWorld *GetInstance();

		RCAPI RecastWorld();
		RCAPI ~RecastWorld() override;

	protected:
		void Update(float delta) override;
			
	private:
		static RecastWorld *_instance;
			
		RNDeclareMetaAPI(RecastWorld, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTWORLD_H_) */
