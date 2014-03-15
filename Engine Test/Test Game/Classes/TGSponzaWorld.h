//
//  TGSponzaWorld.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Test_Game__TGSponzaWorld__
#define __Test_Game__TGSponzaWorld__

#include <Rayne.h>
#include "TGWorld.h"

#define TGSunTag   1
#define TGLightTag 2

namespace TG
{
	class SponzaWorld : public World
	{
	public:
		SponzaWorld();
		
		void LoadOnThread(RN::Thread *thread, RN::Deserializer *deserializer) override;
		void SaveOnThread(RN::Thread *thread, RN::Serializer *serializer) override;
		
		RNDeclareMetaWithTraits(SponzaWorld, World, RN::MetaClassTraitCronstructable)
	};
}

#endif /* defined(__Test_Game__TGSponzaWorld__) */
