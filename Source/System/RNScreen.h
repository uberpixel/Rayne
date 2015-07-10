//
//  RNScreen.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SCREEN_H_
#define __RAYNE_SCREEN_H_

#include "../Base/RNBase.h"
#include "../Math/RNRect.h"
#include "../Math/RNVector.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"
#include "../Objects/RNValue.h"

namespace RN
{
	class Kernel;
	class Screen : public Object
	{
	public:
		friend class Kernel;

		~Screen();

		uint32 GetWidth() const { return static_cast<uint32>(_frame.width); }
		uint32 GetHeight() const { return static_cast<uint32>(_frame.height); }

		float GetScaleFactor() const { return _scaleFactor; }
		const Rect &GetFrame() const { return _frame; }
		const Array *GetSupportedResolutions() const { return _resolutions; } // Array of RN::Value of RN::Vector2
		const String *GetName() const { return _name; }
		bool IsMainScreen() const { return _isMainScreen; }

		static Screen *GetMainScreen();
		static Array *GetScreens();

#if RN_PLATFORM_MAC_OS
		CGDirectDisplayID GetDisplayID() const { return _display; }
		void *GetNSScreen() const { return _nsscreen; }
#endif

	private:
		static void InitializeScreens();
		static void TeardownScreens();

#if RN_PLATFORM_MAC_OS
		Screen(CGDirectDisplayID display);
#endif

		Rect _frame;
		float _scaleFactor;
		Array *_resolutions;
		String *_name;
		bool _isMainScreen;

#if RN_PLATFORM_MAC_OS
		CGDirectDisplayID _display;
		void *_nsscreen; // NSScreen * instance
#endif

		RNDeclareMeta(Screen)
	};
}


#endif /* __RAYNE_SCREEN_H_ */
