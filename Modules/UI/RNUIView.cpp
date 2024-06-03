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
			_clipToBounds(true),
			_isHidden(false),
			_isHiddenByParent(false),
			_needsMeshUpdate(true),
			_subviews(new Array()),
			_superview(nullptr),
			_backgroundColor{Color::ClearColor(), Color::ClearColor(), Color::ClearColor(), Color::ClearColor()},
			_hasVertexColors(false),
			_inheritRenderSettings(true),
			_isDepthWriteEnabled(false),
			_isColorWriteEnabled(true),
			_isAlphaWriteEnabled(false),
			_depthMode(DepthMode::GreaterOrEqual),
			_depthOffset(200.0f),
			_depthFactor(50.0f),
			_opacityFactor(1.0f),
			_combinedOpacityFactor(1.0f),
			_blendSourceFactorRGB(BlendFactor::SourceAlpha),
			_blendDestinationFactorRGB(BlendFactor::OneMinusSourceAlpha),
			_blendOperationRGB(BlendOperation::Add),
			_blendSourceFactorA(BlendFactor::One),
			_blendDestinationFactorA(BlendFactor::Zero),
			_blendOperationA(BlendOperation::Add),
			_cornerRadius(0.0f, 0.0f, 0.0f, 0.0f),
			_isCircle(false),
			_renderPriorityOverride(0)
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
			subview->SetRenderGroupForAll(GetRenderGroup());
			subview->SetOpacityFromParent(_combinedOpacityFactor);
			
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
				if(!GetSceneInfo())
				{
					if(_renderPriorityOverride != 0)
					{
						SetRenderPriority(_renderPriorityOverride);
					}
					
					if(_superview)
					{
						if(_renderPriorityOverride == 0) SetRenderPriority(_superview->GetRenderPriority() + 1);
						if(_inheritRenderSettings) _depthMode = _superview->_depthMode;
					}
				}
			}
			
			SceneNode::WillUpdate(changeSet);
		}
	
		void View::HandleButtonClick()
		{
			_subviews->Enumerate<View>([](View *view, size_t index, bool &stop){
				if(!view->GetIsHidden() && view->_combinedOpacityFactor > RN::k::EpsilonFloat) view->HandleButtonClick();
			});
		}
			
		void View::HandleButtonClickLate()
		{
			_subviews->Enumerate<View>([](View *view, size_t index, bool &stop){
				if(!view->GetIsHidden() && view->_combinedOpacityFactor > RN::k::EpsilonFloat) view->HandleButtonClickLate();
			});
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
			
			RN::Vector3 newPosition(_frame.x, -_frame.y, 0.0f);
			if(_superview)
			{
				newPosition.x = _superview->_bounds.x + newPosition.x;
				newPosition.y = -_superview->_bounds.y + newPosition.y;
			}
			SetPosition(newPosition);

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
				child->SetPosition(RN::Vector3(_bounds.x + child->_frame.x, -_bounds.y - child->_frame.y, 0.0f));
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
			if(_hasVertexColors)
			{
				Unlock();
				SetBackgroundColor(color, color, color, color);
				return;
			}
			
			_backgroundColor[0] = color;
			
			RN::Model *model = GetModel();
			if(model)
			{
				Color finalColor = _backgroundColor[0];
				finalColor.a *= _combinedOpacityFactor;
				
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDiffuseColor(finalColor);
				material->SetSkipRendering(finalColor.a < k::EpsilonFloat);
			}
			Unlock();
		}
	
		void View::SetBackgroundColor(const Color &colorTopLeft, const Color &colorTopRight, const Color &colorBottomLeft, const Color &colorBottomRight)
		{
			Lock();
			RN::Model *model = GetModel();
			RN_ASSERT(!model || _hasVertexColors, "Background color with gradient has to be set before the view is rendered the first time.");
			
			_hasVertexColors = true;
			_backgroundColor[0] = colorTopLeft;
			_backgroundColor[1] = colorTopRight;
			_backgroundColor[2] = colorBottomLeft;
			_backgroundColor[3] = colorBottomRight;
			_needsMeshUpdate = true;
			Unlock();
		}
	
		void View::SetOpacity(float opacity)
		{
			Lock();
			_opacityFactor = opacity;
			float parentOpacity = _superview? _superview->_combinedOpacityFactor : 1.0f;
			Unlock();
			
			SetOpacityFromParent(parentOpacity);
		}
		
		void View::SetDepthModeAndWrite(DepthMode depthMode, bool writeDepth, float depthFactor, float depthOffset, bool colorWrite, bool alphaWrite)
		{
			Lock();
			_inheritRenderSettings = false;
			_depthMode = depthMode;
			_isDepthWriteEnabled = writeDepth;
			_isColorWriteEnabled = colorWrite;
			_isAlphaWriteEnabled = alphaWrite;
			_depthOffset = depthOffset;
			_depthFactor = depthFactor;
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetColorWriteMask(_isColorWriteEnabled, _isColorWriteEnabled, _isColorWriteEnabled, _isAlphaWriteEnabled);
				material->SetDepthMode(_depthMode);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
			}
			Unlock();
		}
	
		void View::SetBlending(BlendFactor sourceFactorRGB, BlendFactor destinationFactorRGB, BlendOperation operationRGB, BlendFactor sourceFactorA, BlendFactor destinationFactorA, BlendOperation operationA)
		{
			Lock();
			//_inheritRenderSettings = false;
			_blendSourceFactorRGB = sourceFactorRGB;
			_blendDestinationFactorRGB = destinationFactorRGB;
			_blendOperationRGB = operationRGB;
			_blendSourceFactorA = sourceFactorA;
			_blendDestinationFactorA = destinationFactorA;
			_blendOperationA = operationA;
			
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetBlendFactorSource(_blendSourceFactorRGB, _blendSourceFactorA);
				material->SetBlendFactorDestination(_blendDestinationFactorRGB, _blendDestinationFactorA);
				material->SetBlendOperation(_blendOperationRGB, _blendOperationA);
			}
			Unlock();
		}
	
		void View::SetCornerRadius(Vector4 radius)
		{
			_cornerRadius = radius;
			_needsMeshUpdate = true;
		}
	
		void View::SetClipToBounds(bool enabled)
		{
			if(_clipToBounds == enabled) return;
			
			_clipToBounds = enabled;
			CalculateScissorRect();
		}
	
		void View::SetRenderPriorityOverride(int32 renderPriority)
		{
			_renderPriorityOverride = renderPriority;
			RN_DEBUG_ASSERT(!_superview, "Needs to be called BEFORE adding to a superview to work");
		}
	
		void View::SetRenderGroupForAll(uint8 renderGroup)
		{
			SetRenderGroup(renderGroup);
			GetSubviews()->Enumerate<View>([renderGroup](View *view, size_t index, bool &stop){
				view->SetRenderGroupForAll(renderGroup);
			});
		}
	
		void View::SetOpacityFromParent(float parentCombinedOpacity)
		{
			Lock();
			_combinedOpacityFactor = parentCombinedOpacity * _opacityFactor;
			_subviews->Enumerate<View>([&](View *view, size_t index, bool &stop){
				view->SetOpacityFromParent(_combinedOpacityFactor);
			});
			
			if(_hasVertexColors)
			{
				_needsMeshUpdate = true;
				Unlock();
				return;
			}
			
			RN::Model *model = GetModel();
			if(model)
			{
				Color finalColor = _backgroundColor[0];
				finalColor.a *= _combinedOpacityFactor;
				
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDiffuseColor(finalColor);
				material->SetSkipRendering(finalColor.a < k::EpsilonFloat);
			}
			Unlock();
		}
	
		void View::MakeCircle()
		{
			RN_ASSERT(!GetModel(), "MakeCircle can only be called before displaying a view for the first time");
			_isCircle = true;
		}

		// ---------------------
		// MARK: -
		// MARK: Layout
		// ---------------------

		void View::CalculateScissorRect()
		{
			Lock();
			if(_superview && _clipToBounds)
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
				View *child = subviewsCopy->GetObjectAtIndex<View>(i);
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
			
			Mesh *mesh = nullptr;
			
			float maxCornerRadius = std::min(_bounds.width, _bounds.height) * 0.5f;
			Vector4 cornerRadius;
			cornerRadius.x = std::max(std::min(_cornerRadius.x, maxCornerRadius), 0.0f);
			cornerRadius.y = std::max(std::min(_cornerRadius.y, maxCornerRadius), 0.0f);
			cornerRadius.z = std::max(std::min(_cornerRadius.z, maxCornerRadius), 0.0f);
			cornerRadius.w = std::max(std::min(_cornerRadius.w, maxCornerRadius), 0.0f);
			
			if((cornerRadius.x > 0.0f || cornerRadius.y > 0.0f || cornerRadius.z > 0.0f || cornerRadius.w > 0.0f) && !_isCircle)
			{
				float *vertexPositionBuffer = new float[20 * 2];
				float *vertexColorBuffer = _hasVertexColors? new float[20 * 4] : nullptr;
				float *vertexUV0Buffer = new float[20 * 2];
				float *vertexUV1Buffer = new float[20 * 3];
				uint32 *indexBuffer = new uint32[30];
				
				vertexPositionBuffer[0 * 2 + 0] = 0.0f;
				vertexPositionBuffer[0 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[1 * 2 + 0] = cornerRadius.x;
				vertexPositionBuffer[1 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[2 * 2 + 0] = cornerRadius.x;
				vertexPositionBuffer[2 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[3 * 2 + 0] = _frame.width - cornerRadius.y;
				vertexPositionBuffer[3 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[4 * 2 + 0] = _frame.width - cornerRadius.y;
				vertexPositionBuffer[4 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[5 * 2 + 0] = _frame.width;
				vertexPositionBuffer[5 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[6 * 2 + 0] = _frame.width;
				vertexPositionBuffer[6 * 2 + 1] = -cornerRadius.y;
				
				vertexPositionBuffer[7 * 2 + 0] = _frame.width;
				vertexPositionBuffer[7 * 2 + 1] = -cornerRadius.y;
				
				vertexPositionBuffer[8 * 2 + 0] = _frame.width;
				vertexPositionBuffer[8 * 2 + 1] = cornerRadius.w - _frame.height;
				
				vertexPositionBuffer[9 * 2 + 0] = _frame.width;
				vertexPositionBuffer[9 * 2 + 1] = cornerRadius.w - _frame.height;
				
				vertexPositionBuffer[10 * 2 + 0] = _frame.width;
				vertexPositionBuffer[10 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[11 * 2 + 0] = _frame.width - cornerRadius.w;
				vertexPositionBuffer[11 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[12 * 2 + 0] = _frame.width - cornerRadius.w;
				vertexPositionBuffer[12 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[13 * 2 + 0] = cornerRadius.z;
				vertexPositionBuffer[13 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[14 * 2 + 0] = cornerRadius.z;
				vertexPositionBuffer[14 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[15 * 2 + 0] = 0.0f;
				vertexPositionBuffer[15 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[16 * 2 + 0] = 0.0f;
				vertexPositionBuffer[16 * 2 + 1] = cornerRadius.z - _frame.height;
				
				vertexPositionBuffer[17 * 2 + 0] = 0.0f;
				vertexPositionBuffer[17 * 2 + 1] = cornerRadius.z - _frame.height;
				
				vertexPositionBuffer[18 * 2 + 0] = 0.0f;
				vertexPositionBuffer[18 * 2 + 1] = -cornerRadius.x;
				
				vertexPositionBuffer[19 * 2 + 0] = 0.0f;
				vertexPositionBuffer[19 * 2 + 1] = -cornerRadius.x;
				
				if(_hasVertexColors)
				{
					//Top Left
					vertexColorBuffer[0 * 4 + 0] = _backgroundColor[0].r;
					vertexColorBuffer[0 * 4 + 1] = _backgroundColor[0].g;
					vertexColorBuffer[0 * 4 + 2] = _backgroundColor[0].b;
					vertexColorBuffer[0 * 4 + 3] = _backgroundColor[0].a * _combinedOpacityFactor;
					
					vertexColorBuffer[1 * 4 + 0] = _backgroundColor[0].r;
					vertexColorBuffer[1 * 4 + 1] = _backgroundColor[0].g;
					vertexColorBuffer[1 * 4 + 2] = _backgroundColor[0].b;
					vertexColorBuffer[1 * 4 + 3] = _backgroundColor[0].a * _combinedOpacityFactor;
					
					vertexColorBuffer[2 * 4 + 0] = _backgroundColor[0].r;
					vertexColorBuffer[2 * 4 + 1] = _backgroundColor[0].g;
					vertexColorBuffer[2 * 4 + 2] = _backgroundColor[0].b;
					vertexColorBuffer[2 * 4 + 3] = _backgroundColor[0].a * _combinedOpacityFactor;
					
					vertexColorBuffer[18 * 4 + 0] = _backgroundColor[0].r;
					vertexColorBuffer[18 * 4 + 1] = _backgroundColor[0].g;
					vertexColorBuffer[18 * 4 + 2] = _backgroundColor[0].b;
					vertexColorBuffer[18 * 4 + 3] = _backgroundColor[0].a * _combinedOpacityFactor;
					
					vertexColorBuffer[19 * 4 + 0] = _backgroundColor[0].r;
					vertexColorBuffer[19 * 4 + 1] = _backgroundColor[0].g;
					vertexColorBuffer[19 * 4 + 2] = _backgroundColor[0].b;
					vertexColorBuffer[19 * 4 + 3] = _backgroundColor[0].a * _combinedOpacityFactor;
					
					//Top Right
					vertexColorBuffer[3 * 4 + 0] = _backgroundColor[1].r;
					vertexColorBuffer[3 * 4 + 1] = _backgroundColor[1].g;
					vertexColorBuffer[3 * 4 + 2] = _backgroundColor[1].b;
					vertexColorBuffer[3 * 4 + 3] = _backgroundColor[1].a * _combinedOpacityFactor;
					
					vertexColorBuffer[4 * 4 + 0] = _backgroundColor[1].r;
					vertexColorBuffer[4 * 4 + 1] = _backgroundColor[1].g;
					vertexColorBuffer[4 * 4 + 2] = _backgroundColor[1].b;
					vertexColorBuffer[4 * 4 + 3] = _backgroundColor[1].a * _combinedOpacityFactor;
					
					vertexColorBuffer[5 * 4 + 0] = _backgroundColor[1].r;
					vertexColorBuffer[5 * 4 + 1] = _backgroundColor[1].g;
					vertexColorBuffer[5 * 4 + 2] = _backgroundColor[1].b;
					vertexColorBuffer[5 * 4 + 3] = _backgroundColor[1].a * _combinedOpacityFactor;
					
					vertexColorBuffer[6 * 4 + 0] = _backgroundColor[1].r;
					vertexColorBuffer[6 * 4 + 1] = _backgroundColor[1].g;
					vertexColorBuffer[6 * 4 + 2] = _backgroundColor[1].b;
					vertexColorBuffer[6 * 4 + 3] = _backgroundColor[1].a * _combinedOpacityFactor;
					
					vertexColorBuffer[7 * 4 + 0] = _backgroundColor[1].r;
					vertexColorBuffer[7 * 4 + 1] = _backgroundColor[1].g;
					vertexColorBuffer[7 * 4 + 2] = _backgroundColor[1].b;
					vertexColorBuffer[7 * 4 + 3] = _backgroundColor[1].a * _combinedOpacityFactor;
					
					//Bottom Left
					vertexColorBuffer[13 * 4 + 0] = _backgroundColor[2].r;
					vertexColorBuffer[13 * 4 + 1] = _backgroundColor[2].g;
					vertexColorBuffer[13 * 4 + 2] = _backgroundColor[2].b;
					vertexColorBuffer[13 * 4 + 3] = _backgroundColor[2].a * _combinedOpacityFactor;
					
					vertexColorBuffer[14 * 4 + 0] = _backgroundColor[2].r;
					vertexColorBuffer[14 * 4 + 1] = _backgroundColor[2].g;
					vertexColorBuffer[14 * 4 + 2] = _backgroundColor[2].b;
					vertexColorBuffer[14 * 4 + 3] = _backgroundColor[2].a * _combinedOpacityFactor;
					
					vertexColorBuffer[15 * 4 + 0] = _backgroundColor[2].r;
					vertexColorBuffer[15 * 4 + 1] = _backgroundColor[2].g;
					vertexColorBuffer[15 * 4 + 2] = _backgroundColor[2].b;
					vertexColorBuffer[15 * 4 + 3] = _backgroundColor[2].a * _combinedOpacityFactor;
					
					vertexColorBuffer[16 * 4 + 0] = _backgroundColor[2].r;
					vertexColorBuffer[16 * 4 + 1] = _backgroundColor[2].g;
					vertexColorBuffer[16 * 4 + 2] = _backgroundColor[2].b;
					vertexColorBuffer[16 * 4 + 3] = _backgroundColor[2].a * _combinedOpacityFactor;
					
					vertexColorBuffer[17 * 4 + 0] = _backgroundColor[2].r;
					vertexColorBuffer[17 * 4 + 1] = _backgroundColor[2].g;
					vertexColorBuffer[17 * 4 + 2] = _backgroundColor[2].b;
					vertexColorBuffer[17 * 4 + 3] = _backgroundColor[2].a * _combinedOpacityFactor;
					
					//Bottom Right
					vertexColorBuffer[8 * 4 + 0] = _backgroundColor[3].r;
					vertexColorBuffer[8 * 4 + 1] = _backgroundColor[3].g;
					vertexColorBuffer[8 * 4 + 2] = _backgroundColor[3].b;
					vertexColorBuffer[8 * 4 + 3] = _backgroundColor[3].a * _combinedOpacityFactor;
					
					vertexColorBuffer[9 * 4 + 0] = _backgroundColor[3].r;
					vertexColorBuffer[9 * 4 + 1] = _backgroundColor[3].g;
					vertexColorBuffer[9 * 4 + 2] = _backgroundColor[3].b;
					vertexColorBuffer[9 * 4 + 3] = _backgroundColor[3].a * _combinedOpacityFactor;
					
					vertexColorBuffer[10 * 4 + 0] = _backgroundColor[3].r;
					vertexColorBuffer[10 * 4 + 1] = _backgroundColor[3].g;
					vertexColorBuffer[10 * 4 + 2] = _backgroundColor[3].b;
					vertexColorBuffer[10 * 4 + 3] = _backgroundColor[3].a * _combinedOpacityFactor;
					
					vertexColorBuffer[11 * 4 + 0] = _backgroundColor[3].r;
					vertexColorBuffer[11 * 4 + 1] = _backgroundColor[3].g;
					vertexColorBuffer[11 * 4 + 2] = _backgroundColor[3].b;
					vertexColorBuffer[11 * 4 + 3] = _backgroundColor[3].a * _combinedOpacityFactor;
					
					vertexColorBuffer[12 * 4 + 0] = _backgroundColor[3].r;
					vertexColorBuffer[12 * 4 + 1] = _backgroundColor[3].g;
					vertexColorBuffer[12 * 4 + 2] = _backgroundColor[3].b;
					vertexColorBuffer[12 * 4 + 3] = _backgroundColor[3].a * _combinedOpacityFactor;
				}
				
				for(int i = 0; i < 20; i++)
				{
					vertexUV0Buffer[i * 2 + 0] = vertexPositionBuffer[i * 2 + 0] / _frame.width;
					vertexUV0Buffer[i * 2 + 1] = -vertexPositionBuffer[i * 2 + 1] / _frame.height;
					
					vertexUV1Buffer[i * 3 + 0] = 0.0f;
					vertexUV1Buffer[i * 3 + 1] = 1.0f;
					vertexUV1Buffer[i * 3 + 2] = 1.0f;
				}
				
				vertexUV1Buffer[19 * 3 + 0] = 0.0f;
				vertexUV1Buffer[19 * 3 + 1] = 0.0f;
				vertexUV1Buffer[0 * 3 + 0] = 0.5f;
				vertexUV1Buffer[0 * 3 + 1] = 0.0f;
				vertexUV1Buffer[1 * 3 + 0] = 1.0f;
				vertexUV1Buffer[1 * 3 + 1] = 1.0f;
				
				vertexUV1Buffer[4 * 3 + 0] = 0.0f;
				vertexUV1Buffer[4 * 3 + 1] = 0.0f;
				vertexUV1Buffer[5 * 3 + 0] = 0.5f;
				vertexUV1Buffer[5 * 3 + 1] = 0.0f;
				vertexUV1Buffer[6 * 3 + 0] = 1.0f;
				vertexUV1Buffer[6 * 3 + 1] = 1.0f;
				
				vertexUV1Buffer[9 * 3 + 0] = 0.0f;
				vertexUV1Buffer[9 * 3 + 1] = 0.0f;
				vertexUV1Buffer[10 * 3 + 0] = 0.5f;
				vertexUV1Buffer[10 * 3 + 1] = 0.0f;
				vertexUV1Buffer[11 * 3 + 0] = 1.0f;
				vertexUV1Buffer[11 * 3 + 1] = 1.0f;
				
				vertexUV1Buffer[14 * 3 + 0] = 0.0f;
				vertexUV1Buffer[14 * 3 + 1] = 0.0f;
				vertexUV1Buffer[15 * 3 + 0] = 0.5f;
				vertexUV1Buffer[15 * 3 + 1] = 0.0f;
				vertexUV1Buffer[16 * 3 + 0] = 1.0f;
				vertexUV1Buffer[16 * 3 + 1] = 1.0f;
				
				indexBuffer[0] = 0;
				indexBuffer[1] = 1;
				indexBuffer[2] = 19;
				
				indexBuffer[3] = 2;
				indexBuffer[4] = 17;
				indexBuffer[5] = 18;
				
				indexBuffer[6] = 2;
				indexBuffer[7] = 13;
				indexBuffer[8] = 17;
				
				indexBuffer[9] = 14;
				indexBuffer[10] = 15;
				indexBuffer[11] = 16;
				
				indexBuffer[12] = 2;
				indexBuffer[13] = 3;
				indexBuffer[14] = 13;
				
				indexBuffer[15] = 3;
				indexBuffer[16] = 12;
				indexBuffer[17] = 13;
				
				indexBuffer[18] = 3;
				indexBuffer[19] = 7;
				indexBuffer[20] = 12;
				
				indexBuffer[21] = 7;
				indexBuffer[22] = 8;
				indexBuffer[23] = 12;
				
				indexBuffer[24] = 4;
				indexBuffer[25] = 5;
				indexBuffer[26] = 6;
				
				indexBuffer[27] = 9;
				indexBuffer[28] = 10;
				indexBuffer[29] = 11;
				
				std::vector<Mesh::VertexAttribute> meshVertexAttributes;
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2);
				if(_hasVertexColors) meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Color0, PrimitiveType::Color);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector3);
				
				mesh = new Mesh(meshVertexAttributes, 20, 30);
				mesh->BeginChanges();
				
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				if(_hasVertexColors) mesh->SetElementData(Mesh::VertexAttribute::Feature::Color0, vertexColorBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUV0Buffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords1, vertexUV1Buffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
				
				mesh->EndChanges();
				
				delete[] vertexPositionBuffer;
				if(_hasVertexColors) delete[] vertexColorBuffer;
				delete[] vertexUV0Buffer;
				delete[] vertexUV1Buffer;
				delete[] indexBuffer;
			}
			else
			{
				float *vertexPositionBuffer = new float[4 * 2];
				float *vertexColorBuffer = _hasVertexColors? new float[4 * 4] : nullptr;
				float *vertexUVBuffer = new float[4 * 2];
				uint32 *indexBuffer = new uint32[6];
				
				vertexPositionBuffer[0 * 2 + 0] = 0.0f;
				vertexPositionBuffer[0 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[1 * 2 + 0] = _frame.width;
				vertexPositionBuffer[1 * 2 + 1] = 0.0f;
				
				vertexPositionBuffer[2 * 2 + 0] = _frame.width;
				vertexPositionBuffer[2 * 2 + 1] = -_frame.height;
				
				vertexPositionBuffer[3 * 2 + 0] = 0.0f;
				vertexPositionBuffer[3 * 2 + 1] = -_frame.height;
				
				if(_hasVertexColors)
				{
					vertexColorBuffer[0 * 4 + 0] = _backgroundColor[0].r;
					vertexColorBuffer[0 * 4 + 1] = _backgroundColor[0].g;
					vertexColorBuffer[0 * 4 + 2] = _backgroundColor[0].b;
					vertexColorBuffer[0 * 4 + 3] = _backgroundColor[0].a * _combinedOpacityFactor;
					
					vertexColorBuffer[1 * 4 + 0] = _backgroundColor[1].r;
					vertexColorBuffer[1 * 4 + 1] = _backgroundColor[1].g;
					vertexColorBuffer[1 * 4 + 2] = _backgroundColor[1].b;
					vertexColorBuffer[1 * 4 + 3] = _backgroundColor[1].a * _combinedOpacityFactor;
					
					vertexColorBuffer[2 * 4 + 0] = _backgroundColor[3].r;
					vertexColorBuffer[2 * 4 + 1] = _backgroundColor[3].g;
					vertexColorBuffer[2 * 4 + 2] = _backgroundColor[3].b;
					vertexColorBuffer[2 * 4 + 3] = _backgroundColor[3].a * _combinedOpacityFactor;
					
					vertexColorBuffer[3 * 4 + 0] = _backgroundColor[2].r;
					vertexColorBuffer[3 * 4 + 1] = _backgroundColor[2].g;
					vertexColorBuffer[3 * 4 + 2] = _backgroundColor[2].b;
					vertexColorBuffer[3 * 4 + 3] = _backgroundColor[2].a * _combinedOpacityFactor;
				}
				
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
				if(_hasVertexColors) meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Color0, PrimitiveType::Color);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
				
				mesh = new Mesh(meshVertexAttributes, 4, 6);
				mesh->BeginChanges();
				
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				if(_hasVertexColors) mesh->SetElementData(Mesh::VertexAttribute::Feature::Color0, vertexColorBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUVBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
				
				mesh->EndChanges();
				
				delete[] vertexPositionBuffer;
				if(_hasVertexColors) delete[] vertexColorBuffer;
				delete[] vertexUVBuffer;
				delete[] indexBuffer;
			}
			
			Model *model = GetModel();
			if(!model)
			{
				Material *material = Material::WithShaders(nullptr, nullptr);
				Shader::Options *shaderOptions = Shader::Options::WithNone();
				shaderOptions->EnableAlpha();
				shaderOptions->AddDefine("RN_UI", "1");
				if((_cornerRadius.x > 0.0f || _cornerRadius.y > 0.0f || _cornerRadius.z > 0.0f || _cornerRadius.w > 0.0f) && !_isCircle) shaderOptions->AddDefine("RN_UV1", "1");
				if(_hasVertexColors) shaderOptions->AddDefine("RN_COLOR", "1");
				if(_isCircle) shaderOptions->AddDefine("RN_UI_CIRCLE", "1");
				material->SetAlphaToCoverage(false);
				material->SetCullMode(CullMode::None);
				material->SetDepthMode(_depthMode);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetColorWriteMask(_isColorWriteEnabled, _isColorWriteEnabled, _isColorWriteEnabled, _isAlphaWriteEnabled);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
				material->SetBlendFactorSource(_blendSourceFactorRGB, _blendSourceFactorA);
				material->SetBlendFactorDestination(_blendDestinationFactorRGB, _blendDestinationFactorA);
				material->SetBlendOperation(_blendOperationRGB, _blendOperationA);
				if(!_hasVertexColors)
				{
					Color finalColor = _backgroundColor[0];
					finalColor.a *= _combinedOpacityFactor;
					material->SetSkipRendering(finalColor.a < k::EpsilonFloat);
					material->SetDiffuseColor(finalColor);
				}
				else
				{
					material->SetSkipRendering(false);
					material->SetDiffuseColor(RN::Color::White());
				}

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
