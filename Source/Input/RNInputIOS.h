//
//  RNInputIOS.h
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INPUTIOS_H_
#define __RAYNE_INPUTIOS_H_

#include "RNInputManager.h"

#if RN_PLATFORM_IOS
namespace RN
{
	class InputIOS
	{
	public:
		InputIOS(InputManager *manager);
		~InputIOS();

		// Called by the touch view on the main thread.
		void HandleTouches(void *touches, InputManager::TouchPhase phase);
		void SetActive(bool active);

	private:
		InputManager *_manager;
		void *_view;
	};
}
#endif


#endif /* __RAYNE_INPUTIOS_H_ */
