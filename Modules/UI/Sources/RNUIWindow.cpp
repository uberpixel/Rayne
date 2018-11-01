//
//  RNUIWindow.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIWindow.h"
#include "RNUIServer.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Window, Object)

		Window::Window(Style style, const Rect &frame) :
			_frame(frame),
			_server(nullptr),
			_backingStore(nullptr),
			_needsNewBackingStore(true),
			_model(nullptr),
			_mesh(nullptr),
			_material(nullptr),
			_drawable(nullptr)
		{
			_contentView = new View(Rect(0, 0, frame.width, frame.height));
			_contentView->_window = this;
			_contentView->SetBackgroundColor(Color::White());
		}

		Window::~Window()
		{
			SafeRelease(_backingStore);
			SafeRelease(_contentView);
			SafeRelease(_model);
		}


		Vector2 Window::GetContentSize() const
		{
			return _frame.GetSize();
		}
		View *Window::GetContentView() const
		{
			return _contentView;
		}



		void Window::Open(Server *server)
		{
			if(!server)
				server = Server::GetDefaultServer();

			server->AddWindow(this);
		}
		void Window::Close()
		{
			if(_server)
				_server->RemoveWindow(this);
		}


		Mesh *Window::CreateMesh() const
		{
			Vector2 halfSize = GetContentSize() * 0.5f;
			return Mesh::WithTexturedPlane(Quaternion(RN::Vector3(0.0f, 90.0f, 0.0f)), Vector3(halfSize.x, halfSize.y, 0.0f), halfSize);
		}

		void Window::Update()
		{
			if(_needsNewBackingStore)
			{
				SafeRelease(_backingStore);

				Vector2 size = GetContentSize();
				_backingStore = new Context(static_cast<size_t>(size.x), static_cast<size_t>(size.y), true);
			}
			
			if(_server)
			{
				if(!_drawable)
				{
					_model = new Model();
					_model->AddLODStage(_model->GetDefaultLODFactors()[0]);
					
					_drawable = Renderer::GetActiveRenderer()->CreateDrawable();
					
					SafeRelease(_mesh);
					_mesh = CreateMesh()->Retain();
					
					SafeRelease(_material);
					_material = Material::WithShaders(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, Shader::Options::WithMesh(_mesh)), Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, Shader::Options::WithMesh(_mesh)));
					_material->SetDepthWriteEnabled(false);
					_material->SetDepthMode(DepthMode::Always);
					
					Model::LODStage *stage = _model->GetLODStage(0);
					stage->AddMesh(_mesh, _material);
				}
				
				if(_needsNewBackingStore)
				{
					_material->SetTextures(RN::Array::WithObjects({_backingStore->GetTexture()}));
					_drawable->Update(_mesh, _material, nullptr);
				}
				
				float height = _server->GetHeight();
				Vector3 translation = Vector3(_frame.x, (height - _frame.height) - _frame.y, 0);
				
				_drawable->modelMatrix = Matrix::WithTranslation(translation);
				_drawable->inverseModelMatrix = _drawable->modelMatrix.GetInverse();
			}
			
			_needsNewBackingStore = false;
			_contentView->LayoutIfNeeded();
		}
		
		void Window::DrawViews()
		{
			_contentView->__DrawInContext(_backingStore);
			_backingStore->UpdateTexture(false);
		}

		void Window::Render(Renderer *renderer)
		{
			DrawViews();
			renderer->SubmitDrawable(_drawable);
		}
	}
}
