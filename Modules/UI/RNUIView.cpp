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
			_isHidden(false),
			_isHiddenByParent(false),
			_needsMeshUpdate(true),
			_subviews(new Array()),
			_superview(nullptr),
			_backgroundColor(Color::ClearColor()),
			_isDepthWriteEnabled(false),
			_depthMode(DepthMode::GreaterOrEqual),
			_depthOffset(-200.0f),
			_depthFactor(-50.0f)
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
			Lock();
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->_superview = nullptr;
			}

			SafeRelease(_subviews);
			Unlock();
		}


		// ---------------------
		// MARK: -
		// MARK: Coordinate systems
		// ---------------------

		void View::ConvertPointToWindow(Vector2 &point) const
		{
			const View *view = this;
			while(view->_superview)
			{
				point.x += view->_frame.x + view->_bounds.x;
				point.y += view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}
			
			point.x += view->_bounds.x;
			point.y += view->_bounds.y;
		}

		void View::ConvertPointFromWindow(Vector2 &point) const
		{
			const View *view = this;
			while(view->_superview)
			{
				point.x -= view->_frame.x + view->_bounds.x;
				point.y -= view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}
			
			point.x -= view->_bounds.x;
			point.y -= view->_bounds.y;
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
			
			const View *view = this;
			while(view)
			{
				converted.x += view->_frame.x + view->_bounds.x;
				converted.y += view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}

			return converted;
		}

		Vector2 View::ConvertPointFromBase(const Vector2 &point) const
		{
			Vector2 converted = point;
			
			const View *view = this;
			while(view)
			{
				converted.x -= view->_frame.x + view->_bounds.x;
				converted.y -= view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}

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

		void View::AddSubview(View *subview)
		{
			Lock();
			subview->Retain();

			if(subview->_superview)
				subview->RemoveFromSuperview();

			subview->WillMoveToSuperview(this);

			_subviews->AddObject(subview);
			subview->_superview = this;
			
			AddChild(subview);
			
			subview->CalculateScissorRect();

			subview->DidMoveToSuperview(this);
			subview->Release();

			DidAddSubview(subview);
			Unlock();
		}

		void View::RemoveSubview(View *subview)
		{
			Lock();
			size_t index = _subviews->GetIndexOfObject(subview);
			if(index != kRNNotFound)
			{
				WillRemoveSubview(subview);

				subview->Retain();
				subview->WillMoveToSuperview(nullptr);

				_subviews->RemoveObjectAtIndex(index);
				subview->RemoveFromParent();

				subview->_superview = nullptr;

				subview->DidMoveToSuperview(nullptr);
				subview->Release();
			}
			Unlock();
		}

		void View::RemoveAllSubviews()
		{
			Lock();
			size_t count = _subviews->GetCount();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews->GetObjectAtIndex<View>(i);

				WillRemoveSubview(subview);
				subview->WillMoveToSuperview(nullptr);
				subview->RemoveFromParent();

				subview->_superview = nullptr;

				subview->DidMoveToSuperview(nullptr);
			}

			_subviews->RemoveAllObjects();
			Unlock();
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

				Lock();
				_subviews->RemoveObject(subview);
				_subviews->AddObject(subview);
				Unlock();

				subview->Release();
				DidBringSubviewToFront(subview);
			}
		}

		void View::SendSubviewToBack(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();

				Lock();
				if(_subviews->GetCount() > 1)
				{
					_subviews->RemoveObject(subview);
					_subviews->InsertObjectAtIndex(subview, 0);
				}
				Unlock();

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
	
		void View::WillUpdate(ChangeSet changeSet)
		{
			if(changeSet == ChangeSet::World)
			{
				if(_superview && !GetSceneInfo())
				{
					SetRenderPriority(_superview->GetRenderPriority() + 1);
				}
			}
			
			SceneNode::WillUpdate(changeSet);
		}

		// ---------------------
		// MARK: -
		// MARK: Properties
		// ---------------------

		void View::SetFrame(const Rect &frame)
		{
			Lock();
			Vector2 oldSize = _frame.GetSize();

			_frame = frame;
			
			SetPosition(RN::Vector3(_frame.x, -_frame.y, 0.0f));

			_bounds.width  = frame.width;
			_bounds.height = frame.height;
			
			if(oldSize.GetSquaredDistance(_frame.GetSize()) > k::EpsilonFloat)
			{
				_needsMeshUpdate = true;
			}
			Unlock();
			
			CalculateScissorRect();
		}

		void View::SetBounds(const Rect &bounds)
		{
			Lock();
			_bounds = bounds;
			//_needsMeshUpdate = true;
			
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->SetPosition(RN::Vector3(_bounds.x + child->GetFrame().x, -_bounds.y - child->GetFrame().y, 0.0f));
			}
			Unlock();
			
			CalculateScissorRect();
		}
	
		void View::SetHidden(bool hidden)
		{
			Lock();
			_isHidden = hidden;
			Unlock();
		}

		void View::SetBackgroundColor(const Color &color)
		{
			Lock();
			_backgroundColor = color;
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDiffuseColor(_backgroundColor);
				material->SetSkipRendering(_backgroundColor.a < k::EpsilonFloat);
			}
			Unlock();
		}
		
		void View::SetDepthModeAndWrite(DepthMode depthMode, bool writeDepth, float depthFactor, float depthOffset)
		{
			Lock();
			_depthMode = depthMode;
			_isDepthWriteEnabled = writeDepth;
			_depthOffset = depthOffset;
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetDepthMode(_depthMode);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
			}
			Unlock();
		}

		// ---------------------
		// MARK: -
		// MARK: Layout
		// ---------------------

		void View::CalculateScissorRect()
		{
			Lock();
			if(_superview)
			{
				RN::Rect parentScissorRect;
				parentScissorRect.x = -_superview->_bounds.x - _frame.x;
				parentScissorRect.y = -_superview->_bounds.y - _frame.y;
				parentScissorRect.width  = _superview->_frame.width;
				parentScissorRect.height = _superview->_frame.height;
				
				RN::Rect parentParentScissorRect = _superview->GetScissorRect();
				parentParentScissorRect.x -= _superview->_bounds.x + _frame.x;
				parentParentScissorRect.y -= _superview->_bounds.y + _frame.y;
				
				float right = std::min(parentScissorRect.GetRight(), parentParentScissorRect.GetRight());
				float bottom = std::min(parentScissorRect.GetBottom(), parentParentScissorRect.GetBottom());
				
				_scissorRect.x = std::max(parentScissorRect.x, parentParentScissorRect.x);
				_scissorRect.y = std::max(parentScissorRect.y, parentParentScissorRect.y);
				
				_scissorRect.width = std::max(right - parentScissorRect.x, 0.0f);
				_scissorRect.height = std::max(bottom - parentScissorRect.y, 0.0f);
			}
			else
			{
				_scissorRect.x = 0.0f;
				_scissorRect.y = 0.0f;
				_scissorRect.width  = _frame.width;
				_scissorRect.height = _frame.height;
			}
			
			Model *model = GetModel();
			if(model)
			{
				Model::LODStage *lodStage = model->GetLODStage(0);
				for(int i = 0; i < lodStage->GetCount(); i++)
				{
					lodStage->GetMaterialAtIndex(i)->SetCustomShaderUniform(RNCSTR("uiClippingRect"), Value::WithVector4(Vector4(_scissorRect.GetLeft(), _scissorRect.GetRight(), _scissorRect.GetTop(), _scissorRect.GetBottom())));
				}
			}
			
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->CalculateScissorRect();
			}
			
			Unlock();
		}
	
		bool View::UpdateCursorPosition(const Vector2 &cursorPosition)
		{
			bool needsRedraw = false;
			
			Lock();
			//Copy to prevent multithreading issues with adding/removing/moving subviews
			//TODO: This is called often, instead _subviews should be updated asynchroneously
			Array *subviewsCopy = _subviews->Copy();
			Unlock();
			
			// Update all children
			size_t count = subviewsCopy->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				if(child->UpdateCursorPosition(cursorPosition))
				{
					needsRedraw = true;
				}
			}
			
			subviewsCopy->Release();
			
			return needsRedraw;
		}
	
		void View::UpdateModel()
		{
			Lock();
			
			float *vertexPositionBuffer = new float[4 * 2];
			float *vertexUVBuffer = new float[4 * 2];
			
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
			
			vertexUVBuffer[0 * 2 + 0] = 0.0f;
			vertexUVBuffer[0 * 2 + 1] = 0.0f;
			
			vertexUVBuffer[1 * 2 + 0] = 1.0f;
			vertexUVBuffer[1 * 2 + 1] = 0.0f;
			
			vertexUVBuffer[2 * 2 + 0] = 1.0f;
			vertexUVBuffer[2 * 2 + 1] = 1.0f;
			
			vertexUVBuffer[3 * 2 + 0] = 0.0f;
			vertexUVBuffer[3 * 2 + 1] = 1.0f;
		
			indexBuffer[0] = 0;
			indexBuffer[1] = 3;
			indexBuffer[2] = 1;
			
			indexBuffer[3] = 3;
			indexBuffer[4] = 2;
			indexBuffer[5] = 1;
			
			std::vector<Mesh::VertexAttribute> meshVertexAttributes;
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2);
			meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
			
			Mesh *mesh = new Mesh(meshVertexAttributes, 4, 6);
			mesh->BeginChanges();
			
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
			mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUVBuffer);
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
			
			mesh->EndChanges();

			delete[] vertexPositionBuffer;
			delete[] vertexUVBuffer;
			delete[] indexBuffer;
			
			Model *model = GetModel();
			if(!model)
			{
				Material *material = Material::WithShaders(nullptr, nullptr);
				Shader::Options *shaderOptions = Shader::Options::WithNone();
				shaderOptions->EnableAlpha();
				shaderOptions->AddDefine(RNCSTR("RN_UI"), RNCSTR("1"));
				material->SetAlphaToCoverage(false);
				material->SetCullMode(CullMode::None);
				material->SetDepthMode(_depthMode);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
				material->SetBlendOperation(BlendOperation::Add, BlendOperation::Add);
				material->SetBlendFactorSource(BlendFactor::SourceAlpha, BlendFactor::SourceAlpha);
				material->SetBlendFactorDestination(BlendFactor::OneMinusSourceAlpha, BlendFactor::OneMinusSourceAlpha);
				material->SetSkipRendering(_backgroundColor.a < k::EpsilonFloat);
				material->SetDiffuseColor(_backgroundColor);

				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions));
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions));
				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);

				material->SetCustomShaderUniform(RNCSTR("uiClippingRect"), Value::WithVector4(Vector4(_scissorRect.GetLeft(), _scissorRect.GetRight(), _scissorRect.GetTop(), _scissorRect.GetBottom())));
				material->SetCustomShaderUniform(RNCSTR("uiOffset"), Value::WithVector2(Vector2(0.0f, 0.0f)));

				model = new Model();
				model->AddLODStage(0.05f)->AddMesh(mesh->Autorelease(), material);
				
				model->Retain();
				SetModel(model->Autorelease());
				model->Release();
			}
			else
			{
				model->GetLODStage(0)->ReplaceMesh(mesh->Autorelease(), 0);
			}
			
			model->CalculateBoundingVolumes();
			SetBoundingBox(model->GetBoundingBox());
			Unlock();
		}

		// ---------------------
		// MARK: -
		// MARK: Drawing
		// ---------------------

		void View::Draw(bool isParentHidden)
		{
			_isHiddenByParent = isParentHidden;
			if(_isHidden || isParentHidden || !_bounds.IntersectsRect(_scissorRect))
			{
				AddFlags(SceneNode::Flags::Hidden);
			}
			else
			{
				RemoveFlags(SceneNode::Flags::Hidden);
			}
			
			if(_needsMeshUpdate && !_isHidden && !isParentHidden)
			{
				UpdateModel();
				_needsMeshUpdate = false;
			}
			
			// Draw all children
			Lock();
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->Draw(_isHidden || isParentHidden);
			}
			Unlock();
		}
	}
}
