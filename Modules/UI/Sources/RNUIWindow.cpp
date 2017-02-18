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
			_mesh(nullptr),
			_material(nullptr)
		{
			_drawable = Renderer::GetActiveRenderer()->CreateDrawable();

			_contentView = new View(Rect(0, 0, frame.width, frame.height));
			_contentView->_window = this;
			_contentView->SetBackgroundColor(Color::White());

			_model = new Model();
			_model->AddLODStage(_model->GetDefaultLODFactors()[0]);
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
			Vector2 size = GetContentSize();

			std::vector<Mesh::VertexAttribute> attributes;

			attributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2);
			attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);

			Mesh *mesh = new Mesh(attributes, 4, 0);
			mesh->SetDrawMode(DrawMode::TriangleStrip);

			mesh->BeginChanges();

			Mesh::Chunk chunk = mesh->GetChunk();

			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(Mesh::VertexAttribute::Feature::Vertices);

			*vertices ++ = Vector2(size.x, size.y);
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(size.x, 0.0f);
			*vertices ++ = Vector2(0.0f, 0.0f);


			Mesh::ElementIterator<Vector2> uvCoords = chunk.GetIterator<Vector2>(Mesh::VertexAttribute::Feature::UVCoords0);

			*uvCoords ++ = Vector2(1.0f, 0.0f);
			*uvCoords ++ = Vector2(0.0f, 0.0f);
			*uvCoords ++ = Vector2(1.0f, 1.0f);
			*uvCoords ++ = Vector2(0.0f, 1.0f);

			mesh->EndChanges();


			return mesh;
		}

		void Window::Update()
		{
			if(_needsNewBackingStore)
			{
				SafeRelease(_backingStore);

				Vector2 size = GetContentSize();
				_backingStore = new Context(static_cast<size_t>(size.x), static_cast<size_t>(size.y), true);


				SafeRelease(_mesh);
				_mesh = CreateMesh();


				ShaderLookupRequest *lookup = new ShaderLookupRequest();

				MaterialDescriptor descriptor;
				descriptor.depthWriteEnabled = false;
				descriptor.depthMode = DepthMode::Always;
				descriptor.AddTexture(_backingStore->GetTexture());
				descriptor.SetShaderProgram(Renderer::GetActiveRenderer()->GetDefaultShader(_mesh, lookup));

				lookup->Release();


				SafeRelease(_material);
				_material = new Material(descriptor);

				{
					Model::LODStage *stage = _model->GetLODStage(0);

					if(stage->GetCount() > 0)
					{
						stage->ReplaceMaterial(_material, 0);
						stage->ReplaceMesh(_mesh, 0);
					}
					else
					{
						stage->AddMesh(_mesh, _material);
					}
				}


				Vector3 translation;

				if(_server)
				{
					float height = _server->GetHeight();
					translation = Vector3(_frame.x, (height - _frame.height) - _frame.y, 0);
				}

				_drawable->Update(_mesh, _material, nullptr);
				_drawable->modelMatrix = Matrix::WithTranslation(translation);
				_drawable->inverseModelMatrix = _drawable->modelMatrix.GetInverse();

				_needsNewBackingStore = false;
			}
			
			_contentView->LayoutIfNeeded();
		}

		void Window::Render(Renderer *renderer)
		{
			_contentView->__DrawInContext(_backingStore);
			_backingStore->UpdateTexture(false);

			renderer->SubmitDrawable(_drawable);
		}
	}
}
