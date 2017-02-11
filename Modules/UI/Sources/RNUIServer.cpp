//
//  RNUIServer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIServer.h"
#include "RNUIWindow.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Server, Object)

		static Server *_defaultServer = nullptr;
		static Server *_mainServer = nullptr;

		Server::Server(Camera *camera) :
			_camera(SafeRetain(camera)),
			_frame(camera->GetFrame())
		{}

		Server::~Server()
		{}

		Server *Server::GetMainServer()
		{
			return _mainServer;
		}
		Server *Server::GetDefaultServer()
		{
			return _defaultServer;
		}

		void Server::MakeDefaultServer()
		{
			SafeRelease(_defaultServer);
			_defaultServer = SafeRetain(this);
		}


		void Server::AddWindow(Window *window)
		{
			RN_ASSERT(window->_server == nullptr, "Window mustn't be part of a server");

			window->_server = this;
			window->Release();

			_windows.push_back(window);
		}
		void Server::RemoveWindow(Window *window)
		{
			RN_ASSERT(window->_server == this, "Window must be part of this server");

			_windows.erase(std::find(_windows.begin(), _windows.end(), window));

			window->_server = nullptr;
			window->Release();
		}
	}
}
