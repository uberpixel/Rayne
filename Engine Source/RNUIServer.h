//
//  RNUIServer.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISERVER_H__
#define __RAYNE_UISERVER_H__

#include "RNBase.h"
#include "RNCamera.h"
#include "RNWidget.h"

namespace RN
{
	class Kernel;
	class UIServer : public Singleton<UIServer>
	{
	friend class Widget;
	friend class Kernel;
	public:
		UIServer();
		virtual ~UIServer();
		
	protected:
		void Render(Renderer *renderer);
		
	private:
		void AddWidget(Widget *widget);
		void RemoveWidget(Widget *widget);
		void MoveWidgetToFront(Widget *widget);
		
		Camera *_camera;
		Rect _frame;
		
		std::deque<Widget *> _widgets;
	};
}

#endif /* __RAYNE_UISERVER_H__ */
