//
//  TGApplication.h
//  Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Game__TGApplication__
#define __Game__TGApplication__

#include <Rayne.h>
#include "TGWorld.h"

namespace TG
{
	class Application : public RN::Application
	{
	public:
		Application();
		~Application();
		
		void Start() override;
		void WillExit() override;
		
		void KeyDown(RN::Event *event) override;
		
	private:
		void LoadLevel(uint32 levelID);
		
		uint32 _currentLevel;
	};
}

#endif /* defined(__Game__TGApplication__) */
