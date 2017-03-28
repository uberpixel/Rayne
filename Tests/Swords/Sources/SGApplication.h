//
//  SGApplication.h
//  Sword Game
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __SWORD_GAME_APPLICATION_H_
#define __SWORD_GAME_APPLICATION_H_

#include <Rayne.h>

using namespace RN::numeric;

namespace SG
{
	class Application : public RN::Application
	{
	public:
		void WillFinishLaunching(RN::Kernel *kernel) override;
		void DidFinishLaunching(RN::Kernel *kernel) override;
	};
}


#endif /* __SWORD_GAME_APPLICATION_H_ */
