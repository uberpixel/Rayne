//
//  RNUIWindow.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIWINDOW_H_
#define __RAYNE_UIWINDOW_H_

#include "RNUIConfig.h"
#include "RNUIContext.h"

namespace RN
{
	namespace UI
	{
		class Server;
		class View;

		class Window : public Object
		{
		public:
			friend class View;
			friend class Server;

			RN_OPTIONS(Style, uint32,
				Borderless = 0,
				Titled = (1 << 0),
				Closable = (1 << 1),
				Minimizable = (1 << 2),
				Maximizable = (1 << 3));

			UIAPI Window(Style style, const Rect &frame);
			UIAPI ~Window();

			UIAPI void SetMinimumSize(const Vector2 &size);
			UIAPI void SetMaximumSize(const Vector2 &size);
			UIAPI void SetFrame(const Rect &frame);
			UIAPI void SetContentSize(const Vector2 &size);
			UIAPI void SetTitle(const String *title);

			UIAPI View *GetContentView() const;
			UIAPI Vector2 GetContentSize() const;
			Server *GetServer() const { return _server; }

			UIAPI void Open(Server *server = nullptr);
			UIAPI void Close();
			UIAPI bool IsOpen() const { return _server != nullptr; }

		private:
			Rect _frame;
			Matrix _transform;

			Server *_server;
			Context *_backingStore;
			bool _needsNewBackingStore;

			RNDeclareMetaAPI(Window, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIWINDOW_H_ */
