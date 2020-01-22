//
//  RNUIServer.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIServer.h"
#include "RNUIWindow.h"
#include "RNUIInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Server, Object)

		static Server *_defaultServer = nullptr;
		static Server *_mainServer = nullptr;

		Server::Server(Camera *camera) :
			_camera(SafeRetain(camera))
		{
			if(!_camera)
			{
				RenderPass *renderPass = new RenderPass();
				renderPass->SetFlags(0);
				renderPass->SetClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
				
				_camera = new Camera(renderPass);
				_camera->SetFlags(Camera::Flags::Orthogonal | Camera::Flags::NoSorting | Camera::Flags::NoDepthWrite);
				_camera->SetClipNear(-500.0f);
			}
		}

		Server::~Server()
		{
			SafeRelease(_camera);
		}

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


		void Server::AddWindow(UI::Window *window)
		{
			RN_ASSERT(window->_server == nullptr, "Window mustn't be part of a server");

			window->_server = this;
			window->Retain();

			_windows.push_back(window);
		}
		void Server::RemoveWindow(UI::Window *window)
		{
			RN_ASSERT(window->_server == this, "Window must be part of this server");

			_windows.erase(std::find(_windows.begin(), _windows.end(), window));

			window->_server = nullptr;
			window->Release();
		}


		void Server::Render(Renderer *renderer)
		{
			for(Window *window : _windows)
				window->Update();

			Rect frame = _camera->GetRenderPass()->GetFrame();
			_camera->SetOrthogonalFrustum(frame.GetBottom(), frame.GetTop(), frame.GetLeft(), frame.GetRight());
			_camera->Update(0.0f);
			_camera->PostUpdate();

/*			renderer->SubmitCamera(_camera, [_windows] {

				for(Window *window : _windows)
					window->Render(renderer);

			});*/
		}

		void Server::InitializeUI()
		{
			SkGraphics::Init();
		}
	}
}
