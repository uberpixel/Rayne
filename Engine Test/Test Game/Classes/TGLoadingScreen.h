//
//  TGLoadingScreen.h
//  Test Game
//
//  Created by Sidney Just on 08/01/14.
//  Copyright (c) 2014 Überpixel. All rights reserved.
//

#ifndef __Test_Game__TGLoadingScreen__
#define __Test_Game__TGLoadingScreen__

#include <Rayne.h>

namespace TG
{
	class LoadingScreen : public RN::UI::Widget
	{
	public:
		LoadingScreen(RN::Progress *progress);
		~LoadingScreen();
		
	private:
		void UpdateSize(RN::Message *message);
		
		RN::UI::ImageView *_backdrop;
		RN::UI::ProgressIndicator *_progressIndicator;
	};
}

#endif /* defined(__Test_Game__TGLoadingScreen__) */
