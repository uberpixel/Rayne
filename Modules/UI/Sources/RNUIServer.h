//
//  RNUIServer.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISERVER_H_
#define __RAYNE_UISERVER_H_

#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		class Window;

		class Server : public Object
		{
		public:
			UIAPI Server(Camera *camera);
			UIAPI ~Server();

			UIAPI static Server *GetMainServer();
			UIAPI static Server *GetDefaultServer();

			UIAPI void MakeDefaultServer();

			UIAPI void AddWindow(Window *window);
			UIAPI void RemoveWindow(Window *window);

			float GetHeight() const { return _frame.height; }
			float GetWidth() const { return _frame.width; }

			Camera *GetCamera() const { return _camera; }

		private:

			Camera *_camera;
			Rect _frame;

			std::deque<Window *> _windows;

			RNDeclareMetaAPI(Server, UIAPI)
		};
	}
}


#endif /* __RAYNE_UISERVER_H_ */
