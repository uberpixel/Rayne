//
//  RNUIServer.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISERVER_H__
#define __RAYNE_UISERVER_H__

#include "RNBase.h"
#include "RNCamera.h"
#include "RNInput.h"
#include "RNUIWidget.h"
#include "RNUIResponder.h"
#include "RNUIControl.h"
#include "RNUIMenu.h"

#define kRNUIServerDidResizeMessage RNCSTR("kRNUIServerDidResizeMessage")

namespace RN
{
	class Kernel;
	class Window;
	
	namespace UI
	{
		class View;
		
		class Server : public ISingleton<Server>
		{
		public:
			friend class Widget;
			friend class RN::Kernel;
			friend class RN::Input;
			friend class RN::Window;
			
			enum class Mode
			{
				Deactivated,
				SingleTracking,
				MultiTracking
			};
			
			RNAPI Server();
			RNAPI ~Server() override;
			
			RNAPI void SetDrawDebugFrames(bool drawDebugFrames);
			RNAPI void SetMainMenu(Menu *menu);
			
			RNAPI uint32 GetHeight() const { return _frame.height; }
			RNAPI uint32 GetWidth() const { return _frame.width; }
			
			RNAPI bool GetDrawDebugFrames() const { return _drawDebugFrames; }
			
			RNAPI Camera *GetCamera() const { return _camera; }
			RNAPI Widget *GetMainWidget() const { return _mainWidget; }
			RNAPI Menu *GetMainMenu() const { return _menu; }
			
#if RN_PLATFORM_MAC_OS
			void PerformMenuBarAction(void *item);
#endif
			
		protected:
			RNAPI void Render(Renderer *renderer);
			
		private:
			void UpdateSize();
			
			void AddWidget(Widget *widget);
			void RemoveWidget(Widget *widget);
			void MoveWidgetToFront(Widget *widget);
			void SortWidgets();
			
			bool ConsumeEvent(Event *event);
			
			void TranslateMenuToPlatform();
			
			Camera *_camera;
			Rect _frame;
			Mode _mode;
			
			Widget *_mainWidget;
			View *_tracking;
			View *_hover;
			std::deque<Widget *> _widgets;
			
			Menu *_menu;
			bool _drawDebugFrames;
			
			RNDeclareSingleton(Server)
		};
	}
}

#endif /* __RAYNE_UISERVER_H__ */
