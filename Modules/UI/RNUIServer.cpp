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

		Server::Server(Camera *camera) :
			_camera(SafeRetain(camera))
		{
			if(!_camera)
			{
				RenderPass *renderPass = new RenderPass();
				//renderPass->SetFlags(0);//RenderPass::Flags::ClearColor | RenderPass::Flags::ClearDepthStencil);
				//renderPass->SetClearColor(Color(1.0f, 0.0f, 0.0f, 1.0f));
				
				_camera = new Camera(renderPass);
				_camera->SetFlags(Camera::Flags::Orthogonal | Camera::Flags::NoDepthWrite | Camera::Flags::RenderLate);
				_camera->SetClipNear(-500.0f);
				_camera->SetRenderGroup(1 << 7);
				
				Rect frame = _camera->GetRenderPass()->GetFrame();
				_camera->SetOrthogonalFrustum(frame.GetBottom(), frame.GetTop(), frame.GetLeft(), frame.GetRight());
				
				_camera->SetWorldPosition(RN::Vector3(0.0f, -frame.GetBottom(), 0.0f));
			}
		}

		Server::~Server()
		{
			SafeRelease(_camera);
		}

		Server *Server::GetDefaultServer()
		{
			if(!_defaultServer)
			{
				_defaultServer = new Server(nullptr);
			}
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
	}
}
