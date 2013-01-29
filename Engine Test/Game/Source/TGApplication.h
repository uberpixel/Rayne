//
//  TGApplication.h
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
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
		virtual ~Application();
		
		virtual void UpdateGame(float delta);
		virtual void UpdateWorld(float delta);
		
	private:
		World *_world;
	};
}

#endif /* defined(__Game__TGApplication__) */
