//
//  RNUIView.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIView.h"
#include "RNUIWindow.h"
#include "RNUIServer.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(View, Entity)

		View::View() :
			_clipsToBounds(true),
			_clipsToWindow(true),
			_isHidden(false),
			_needsMeshUpdate(true),
			_subviews(new Array()),
			_window(nullptr),
			_superview(nullptr),
			_clippingView(nullptr),
			_backgroundColor(Color::ClearColor())
		{
			SetRenderGroup(1 << 7);
			SetRenderPriority(SceneNode::RenderPriority::RenderUI);
		}

		View::View(const Rect &frame) : View()
		{
			SetFrame(frame);
		}
		
		View::~View()
		{
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->_superview = nullptr;
			}

			SafeRelease(_subviews);
		}


		// ---------------------
		// MARK: -
		// MARK: Coordinate systems
		// ---------------------

		void View::ConvertPointToWindow(Vector2 &point) const
		{
			View *view = _superview;
			while(view)
			{
				point.x += view->_frame.x + view->_bounds.x;
				point.y += view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}

			point.x += _frame.x + _bounds.x;
			point.y += _frame.y + _bounds.y;
		}

		void View::ConvertPointFromWindow(Vector2 &point) const
		{
			View *view = _superview;
			while(view)
			{
				point.x -= view->_frame.x + view->_bounds.x;
				point.y -= view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}

			point.x -= _frame.x + _bounds.x;
			point.y -= _frame.y + _bounds.y;
		}

		Vector2 View::ConvertPointToView(const Vector2 &point, View *view) const
		{
			Vector2 converted = point;
			ConvertPointToWindow(converted);

			if(!view)
				return converted;

			view->ConvertPointFromWindow(converted);
			return converted;
		}

		Vector2 View::ConvertPointFromView(const Vector2 &point, View *view) const
		{
			Vector2 converted = point;

			if(view)
				view->ConvertPointToWindow(converted);

			ConvertPointFromWindow(converted);
			return converted;
		}

		Vector2 View::ConvertPointToBase(const Vector2 &point) const
		{
			Vector2 converted = point;
			ConvertPointToWindow(converted);

			if(_window)
			{
				converted.x += _window->_frame.x;
				converted.y += _window->_frame.y;
			}

			return converted;
		}

		Vector2 View::ConvertPointFromBase(const Vector2 &point) const
		{
			Vector2 converted = point;
			if(_window)
			{
				converted.x -= _window->_frame.x;
				converted.y -= _window->_frame.y;
			}

			ConvertPointFromWindow(converted);
			return converted;
		}

		Rect View::ConvertRectToView(const Rect &frame, View *view) const
		{
			Rect converted = frame;
			Vector2 point  = ConvertPointToView(Vector2(frame.x, frame.y), view);

			converted.x = point.x;
			converted.y = point.y;

			return converted;
		}

		Rect View::ConvertRectFromView(const Rect &frame, View *view) const
		{
			Rect converted = frame;
			Vector2 point  = ConvertPointFromView(Vector2(frame.x, frame.y), view);

			converted.x = point.x;
			converted.y = point.y;

			return converted;
		}

		// ---------------------
		// MARK: -
		// MARK: Subviews
		// ---------------------

		void View::ViewHierarchyChanged()
		{
			size_t count = _subviews->GetCount();

			for(size_t i = 0; i < count; i++)
			{
				View *subview = _subviews->GetObjectAtIndex<View>(i);
				subview->_window = _window;
				subview->_clipsToWindow = _clipsToWindow;
				subview->ViewHierarchyChanged();
			}
		}

		void View::AddSubview(View *subview)
		{
			subview->Retain();

			if(subview->_superview)
				subview->RemoveFromSuperview();

			subview->WillMoveToSuperview(this);

			_subviews->AddObject(subview);
			AddChild(subview);

			subview->_superview = this;
			subview->_window = _window;

			subview->ViewHierarchyChanged();
			subview->DidMoveToSuperview(this);
			subview->Release();

			DidAddSubview(subview);
		}

		void View::RemoveSubview(View *subview)
		{
			size_t index = _subviews->GetIndexOfObject(subview);
			if(index != kRNNotFound)
			{
				WillRemoveSubview(subview);

				subview->Retain();
				subview->WillMoveToSuperview(nullptr);

				_subviews->RemoveObjectAtIndex(index);
				subview->RemoveFromParent();

				subview->_superview = nullptr;
				subview->_window = nullptr;

				subview->DidMoveToSuperview(nullptr);
				subview->Release();
			}
		}

		void View::RemoveAllSubviews()
		{
			size_t count = _subviews->GetCount();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews->GetObjectAtIndex<View>(i);

				WillRemoveSubview(subview);
				subview->WillMoveToSuperview(nullptr);
				subview->RemoveFromParent();

				subview->_superview = nullptr;
				subview->_window = nullptr;

				subview->DidMoveToSuperview(nullptr);
			}

			_subviews->RemoveAllObjects();
		}

		void View::RemoveFromSuperview()
		{
			if(!_superview)
				return;

			_superview->RemoveSubview(this);
		}

		void View::BringSubviewToFront(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();

				_subviews->RemoveObject(subview);
				_subviews->AddObject(subview);

				subview->Release();
				DidBringSubviewToFront(subview);
			}
		}

		void View::SendSubviewToBack(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();

				if(_subviews->GetCount() > 1)
				{
					_subviews->RemoveObject(subview);
					_subviews->InsertObjectAtIndex(subview, 0);
				}

				subview->Release();
				DidSendSubviewToBack(subview);
			}
		}

		void View::DidAddSubview(View *subview)
		{}
		void View::WillRemoveSubview(View *subview)
		{}

		void View::DidBringSubviewToFront(View *subview)
		{}
		void View::DidSendSubviewToBack(View *subview)
		{}

		void View::WillMoveToSuperview(View *superview)
		{}
		void View::DidMoveToSuperview(View *superview)
		{}

		// ---------------------
		// MARK: -
		// MARK: Properties
		// ---------------------

		void View::SetFrame(const Rect &frame)
		{
			Vector2 oldSize = _frame.GetSize();

			_frame = frame;
			
			SetPosition(RN::Vector3(_frame.x, -_frame.y, 0.0f));

			_bounds.width  = frame.width;
			_bounds.height = frame.height;
			
			if(oldSize.GetSquaredDistance(_frame.GetSize()) > 0.0f)
			{
				_needsMeshUpdate = true;
			}
		}

		void View::SetBounds(const Rect &bounds)
		{
			_bounds = bounds;
			//_needsMeshUpdate = true;
			
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->SetPosition(RN::Vector3(_bounds.x + child->GetFrame().x, -_bounds.y - child->GetFrame().y, 0.0f));
			}
		}
	
		void View::SetHidden(bool hidden)
		{
			_isHidden = hidden;
		}

		void View::SetBackgroundColor(const Color &color)
		{
			_backgroundColor = color;
			RN::Model *model = GetModel();
			if(model)
			{
				model->GetLODStage(0)->GetMaterialAtIndex(0)->SetDiffuseColor(_backgroundColor);
			}
		}

		// ---------------------
		// MARK: -
		// MARK: Layout
		// ---------------------

		void View::CalculateScissorRect()
		{
			//Vector2 origin = _frame.GetOrigin();

			_clippingView = nullptr;

			/*View *view = _superview;
			while(view)
			{
				if(!_clippingView && view->_clipsToBounds)
					_clippingView = view;

				origin.x += view->_frame.x - view->_bounds.x;
				origin.y += view->_frame.y - view->_bounds.y;

				view = view->_superview;
			}*/

			_scissorRect.x = 0; //origin.x;
			_scissorRect.y = 0; //origin.y;
			_scissorRect.width  = _frame.width;
			_scissorRect.height = _frame.height;

			_scissorRect.x += _clipInsets.left;
			_scissorRect.width -= _clipInsets.left + _clipInsets.right;

			_scissorRect.y += _clipInsets.bottom;
			_scissorRect.height -= _clipInsets.bottom + _clipInsets.top;

			if(_clippingView)
			{
				_scissorRect.x = std::max(_scissorRect.x, _clippingView->_scissorRect.x);
				_scissorRect.width = std::min(_scissorRect.width, _clippingView->_scissorRect.GetRight() - _scissorRect.x);

				_scissorRect.y = std::max(_scissorRect.y, _clippingView->_scissorRect.y);
				_scissorRect.height = std::min(_scissorRect.height, _clippingView->_scissorRect.GetBottom() - _scissorRect.y);
			}
		}
	
		bool View::UpdateCursorPosition(const Vector2 &cursorPosition)
		{
			bool needsRedraw = false;
			
			// Update all children
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				if(child->UpdateCursorPosition(cursorPosition))
				{
					needsRedraw = true;
				}
			}
			
			return needsRedraw;
		}
	
		void View::UpdateModel()
		{
			float *vertexPositionBuffer = new float[4 * 2];
			float *vertexUVBuffer = new float[4 * 3];
			float *vertexColorBuffer = new float[4 * 4];
			
			uint32 *indexBuffer = new uint32[6];
			
			uint32 vertexOffset = 0;
			uint32 indexIndexOffset = 0;
			uint32 indexOffset = 0;
			
			vertexPositionBuffer[0 * 2 + 0] = 0.0f;
			vertexPositionBuffer[0 * 2 + 1] = 0.0f;
			
			vertexPositionBuffer[1 * 2 + 0] = _frame.width;
			vertexPositionBuffer[1 * 2 + 1] = 0.0f;
			
			vertexPositionBuffer[2 * 2 + 0] = _frame.width;
			vertexPositionBuffer[2 * 2 + 1] = -_frame.height;
			
			vertexPositionBuffer[3 * 2 + 0] = 0.0f;
			vertexPositionBuffer[3 * 2 + 1] = -_frame.height;
			
			vertexUVBuffer[0 * 3 + 0] = 1.0f;
			vertexUVBuffer[0 * 3 + 1] = 0.0f;
			vertexUVBuffer[0 * 3 + 2] = -1.0f;
			
			vertexUVBuffer[1 * 3 + 0] = 1.0f;
			vertexUVBuffer[1 * 3 + 1] = 0.0f;
			vertexUVBuffer[1 * 3 + 2] = -1.0f;
			
			vertexUVBuffer[2 * 3 + 0] = 1.0f;
			vertexUVBuffer[2 * 3 + 1] = 0.0f;
			vertexUVBuffer[2 * 3 + 2] = -1.0f;
			
			vertexUVBuffer[3 * 3 + 0] = 1.0f;
			vertexUVBuffer[3 * 3 + 1] = 0.0f;
			vertexUVBuffer[3 * 3 + 2] = -1.0f;
			
			vertexColorBuffer[0 * 4 + 0] = 1.0f;
			vertexColorBuffer[0 * 4 + 1] = 1.0f;
			vertexColorBuffer[0 * 4 + 2] = 1.0f;
			vertexColorBuffer[0 * 4 + 3] = 1.0f;
			
			vertexColorBuffer[1 * 4 + 0] = 1.0f;
			vertexColorBuffer[1 * 4 + 1] = 1.0f;
			vertexColorBuffer[1 * 4 + 2] = 1.0f;
			vertexColorBuffer[1 * 4 + 3] = 1.0f;
			
			vertexColorBuffer[2 * 4 + 0] = 1.0f;
			vertexColorBuffer[2 * 4 + 1] = 1.0f;
			vertexColorBuffer[2 * 4 + 2] = 1.0f;
			vertexColorBuffer[2 * 4 + 3] = 1.0f;
			
			vertexColorBuffer[3 * 4 + 0] = 1.0f;
			vertexColorBuffer[3 * 4 + 1] = 1.0f;
			vertexColorBuffer[3 * 4 + 2] = 1.0f;
			vertexColorBuffer[3 * 4 + 3] = 1.0f;
		
			indexBuffer[0] = 0;
			indexBuffer[1] = 3;
			indexBuffer[2] = 1;
			
			indexBuffer[3] = 3;
			indexBuffer[4] = 2;
			indexBuffer[5] = 1;
			
			std::vector<Mesh::VertexAttribute> meshVertexAttributes;
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2);
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector3);
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Color0, PrimitiveType::Vector4);
			
			Mesh *mesh = new Mesh(meshVertexAttributes, 4, 6);
			mesh->BeginChanges();
			
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
			mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUVBuffer);
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Color0, vertexColorBuffer);
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
			
			mesh->EndChanges();

			delete[] vertexPositionBuffer;
			delete[] vertexUVBuffer;
			delete[] vertexColorBuffer;
			delete[] indexBuffer;
			
			Model *model = GetModel();
			if(!model)
			{
				Material *material = Material::WithShaders(nullptr, nullptr);
				Shader::Options *shaderOptions = Shader::Options::WithMesh(mesh);
				shaderOptions->EnableAlpha();
				shaderOptions->AddDefine(RNCSTR("RN_UI"), RNCSTR("1"));
				material->SetAlphaToCoverage(false);
				material->SetCullMode(CullMode::None);
				//material->SetDepthMode(DepthMode::Always);
				material->SetDepthWriteEnabled(false);
				material->SetBlendOperation(BlendOperation::Add, BlendOperation::Add);
				material->SetBlendFactorSource(BlendFactor::SourceAlpha, BlendFactor::SourceAlpha);
				material->SetBlendFactorDestination(BlendFactor::OneMinusSourceAlpha, BlendFactor::OneMinusSourceAlpha);

				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions));
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions));
				
				material->SetEmissiveColor(Color(-100000.0f, 100000.0f, -100000.0f, 100000.0f));

				model = new Model();
				model->AddLODStage(0.05f)->AddMesh(mesh->Autorelease(), material);
				
				SetModel(model->Autorelease());
			}
			else
			{
				model->GetLODStage(0)->ReplaceMesh(mesh->Autorelease(), 0);
			}
			
			model->CalculateBoundingVolumes();
			SetBoundingBox(model->GetBoundingBox());
			
			model->GetLODStage(0)->GetMaterialAtIndex(0)->SetDiffuseColor(_backgroundColor);
		}

		// ---------------------
		// MARK: -
		// MARK: Drawing
		// ---------------------

		void View::Draw()
		{
			if(_needsMeshUpdate)
			{
				UpdateModel();
				_needsMeshUpdate = false;
			}
			
			// Draw all children
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->Draw();
			}
		}
	}
}
