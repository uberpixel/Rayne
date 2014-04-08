//
//  TGLoadingScreen.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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
