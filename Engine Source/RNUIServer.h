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
#include "RNUIWidget.h"
#include "RNUIResponder.h"
#include "RNUIControl.h"
#include "RNInput.h"

namespace RN
{
	class Kernel;
	
	namespace UI
	{
		class Server : public Singleton<Server>
		{
		public:
			friend class Widget;
			friend class Kernel;
			
			enum class Mode
			{
				Deactivated,
				SingleTracking,
				MultiTracking
			};
			
			Server();
			~Server() override;
			
			void SetDrawDebugFrames(bool drawDebugFrames);
			
			uint32 Height() const { return _frame.height; }
			uint32 Width() const { return _frame.width; }
			
			bool DrawDebugFrames() const { return _drawDebugFrames; }
			
		protected:
			void Render(Renderer *renderer);
			
		private:
			void AddWidget(Widget *widget);
			void RemoveWidget(Widget *widget);
			void MoveWidgetToFront(Widget *widget);
			
			void HandleEvent(Message *event);
			
			Camera *_camera;
			Rect _frame;
			Mode _mode;
			
			Widget *_mainWidget;
			std::deque<Widget *> _widgets;
			
			Widget *_debugWidget;
			bool _drawDebugFrames;
		};
	}
}

#endif /* __RAYNE_UISERVER_H__ */
