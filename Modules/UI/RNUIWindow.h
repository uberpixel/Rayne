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
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class Server;
		class View;

		class Window : public View
		{
		public:
			friend class View;
			friend class Server;

			UIAPI Window(const Rect &frame);
			UIAPI ~Window();

			Server *GetServer() const { return _server; }

			UIAPI void Open(Server *server = nullptr);
			UIAPI void Close();
			UIAPI bool IsOpen() const { return _server != nullptr; }

			UIAPI void Update(float delta) override;

		private:
			Server *_server;

			RNDeclareMetaAPI(Window, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIWINDOW_H_ */
