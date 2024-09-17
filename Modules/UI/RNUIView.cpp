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

#include <KGMeshGeneratorLoopBlinn.h> //Used to generate the outline for views with outline and rounded corners, which requires handling of overlaps

namespace RN
{
	namespace UI
	{
		RNDefineMeta(View, Entity)

		View::View() :
			_clipToBounds(false),
			_isHidden(false),
			_isHiddenByParent(false),
			_needsMeshUpdate(true),
			_subviews(new Array()),
			_superview(nullptr),
			_backgroundColor{Color::ClearColor(), Color::ClearColor(), Color::ClearColor(), Color::ClearColor()},
			_hasBackgroundGradient(false),
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
			_hasOutline(false),
			_outlineThickness(0.0f),
			_renderPriorityOverride(0),
			_renderPriorityOffset(1)
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
			subview->SetPosition(RN::Vector3(_bounds.x + subview->_frame.x, -_bounds.y - subview->_frame.y, 0.0f)); //Update position to respect the new parents bounds
			
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
						if(_renderPriorityOverride == 0) SetRenderPriority(_superview->GetRenderPriority() + _renderPriorityOffset);
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
			if(_frame == frame) return;
			
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
			if(_bounds == bounds) return;
			
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
			if(_hasBackgroundGradient)
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
			RN_ASSERT(!model || _hasBackgroundGradient, "Background color with gradient has to be set before the view is rendered the first time.");
			
			_hasBackgroundGradient = true;
			_backgroundColor[0] = colorTopLeft;
			_backgroundColor[1] = colorTopRight;
			_backgroundColor[2] = colorBottomRight;
			_backgroundColor[3] = colorBottomLeft;

			if(model)
			{
				Color finalColor[4];
				finalColor[0] = _backgroundColor[0];
				finalColor[0].a *= _combinedOpacityFactor;
				finalColor[1] = _backgroundColor[1];
				finalColor[1].a *= _combinedOpacityFactor;
				finalColor[2] = _backgroundColor[2];
				finalColor[2].a *= _combinedOpacityFactor;
				finalColor[3] = _backgroundColor[3];
				finalColor[3].a *= _combinedOpacityFactor;
				
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetDiffuseColor(finalColor[0]);
				material->SetSpecularColor(finalColor[1]);
				material->SetEmissiveColor(finalColor[2]);
				material->SetAmbientColor(finalColor[3]);
				
				material->SetSkipRendering(finalColor[0].a + finalColor[1].a + finalColor[2].a + finalColor[3].a < k::EpsilonFloat);
			}
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
			if(radius == _cornerRadius) return;
			
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
	
		void View::SetRenderPriorityOffset(int32 offset)
		{
			_renderPriorityOffset = offset;
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
			
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				if(!_hasBackgroundGradient)
				{
					Color finalColor = _backgroundColor[0];
					finalColor.a *= _combinedOpacityFactor;
					
					material->SetDiffuseColor(finalColor);
					material->SetSkipRendering(finalColor.a < k::EpsilonFloat);
				}
				else
				{
					Color finalColor[4];
					finalColor[0] = _backgroundColor[0];
					finalColor[0].a *= _combinedOpacityFactor;
					finalColor[1] = _backgroundColor[1];
					finalColor[1].a *= _combinedOpacityFactor;
					finalColor[2] = _backgroundColor[2];
					finalColor[2].a *= _combinedOpacityFactor;
					finalColor[3] = _backgroundColor[3];
					finalColor[3].a *= _combinedOpacityFactor;
					
					material->SetDiffuseColor(finalColor[0]);
					material->SetSpecularColor(finalColor[1]);
					material->SetEmissiveColor(finalColor[2]);
					material->SetAmbientColor(finalColor[3]);
					
					material->SetSkipRendering(finalColor[0].a + finalColor[1].a + finalColor[2].a + finalColor[3].a < k::EpsilonFloat);
				}
			}
			Unlock();
		}
	
		void View::MakeCircle()
		{
			RN_ASSERT(!GetModel(), "MakeCircle can only be called before displaying a view for the first time");
			_isCircle = true;
		}
	
		void View::SetOutline(Color color, float thickness)
		{
			Lock();
			RN::Model *model = GetModel();
			RN_ASSERT(!model || _hasOutline, "SetOutline has to be called before displaying a view for the first time, but can be used to update the values after");
			
			_hasOutline = true;
			_outlineColor = color;
			
			if(thickness != _outlineThickness) _needsMeshUpdate = true;
			_outlineThickness = thickness;

			if(model)
			{
				Color finalColor;
				finalColor = _outlineColor;
				finalColor.a *= _combinedOpacityFactor;
				
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->SetUIOutlineColor(finalColor);
				
				//TODO: Skip rendering stuff should also depend on the outline being visible or not...
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
			RN::Rect oldScissorRect = _scissorRect;
			if(_superview)
			{
				if(_superview->_clipToBounds)
				{
					RN::Rect parentScissorRect;
					parentScissorRect.x = -_superview->_bounds.x - _frame.x;
					parentScissorRect.y = -_superview->_bounds.y - _frame.y;
					parentScissorRect.width  = _superview->_frame.width;
					parentScissorRect.height = _superview->_frame.height;
					
					RN::Rect parentParentScissorRect = _superview->_scissorRect;
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
					_scissorRect = _superview->_scissorRect;
					_scissorRect.x -= _superview->_bounds.x + _frame.x;
					_scissorRect.y -= _superview->_bounds.y + _frame.y;
				}
			}
			else
			{
				_scissorRect.x = 0.0f;
				_scissorRect.y = 0.0f;
				_scissorRect.width  = _frame.width;
				_scissorRect.height = _frame.height;
			}
			
			//Updating all of this tends to be slow, so only do it if the scissor rect actually changed (that didn't work somehow...)
			//if(oldScissorRect != _scissorRect)
			{
				Model *model = GetModel();
				if(model)
				{
					Model::LODStage *lodStage = model->GetLODStage(0);
					for(int i = 0; i < lodStage->GetCount(); i++)
					{
						lodStage->GetMaterialAtIndex(i)->SetUIClippingRect(Vector4(_scissorRect.GetLeft(), _scissorRect.GetRight(), _scissorRect.GetTop(), _scissorRect.GetBottom()));
					}
				}
				
				size_t count = _subviews->GetCount();
				for(size_t i = 0; i < count; i ++)
				{
					View *child = _subviews->GetObjectAtIndex<View>(i);
					child->CalculateScissorRect();
				}
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
				size_t vertexCount = 20;
				size_t indexCount = 30;
				size_t vertexPositionSize = (_hasOutline? 3 : 2);
				
				KG::TriangleMesh outlineTriangleMesh;
				size_t outlineTriangleMeshVertexFloatCount = 0;
				if(_hasOutline && _outlineThickness > RN::k::EpsilonFloat)
				{
					KG::PathCollection paths;
					KG::Path mypath;
					KG::PathSegment segment;
					
					//Outter outline
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.push_back({cornerRadius.x, 0.0});
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, 0.0});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, 0.0});
					segment.controlPoints.push_back({_frame.width, 0.0});
					segment.controlPoints.push_back({_frame.width, -cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width, -cornerRadius.y});
					segment.controlPoints.push_back({_frame.width, -_frame.height + cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width, -_frame.height + cornerRadius.y});
					segment.controlPoints.push_back({_frame.width, -_frame.height});
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, -_frame.height});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, -_frame.height});
					segment.controlPoints.push_back({cornerRadius.x, -_frame.height});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({cornerRadius.x, -_frame.height});
					segment.controlPoints.push_back({0.0, -_frame.height});
					segment.controlPoints.push_back({0.0, -_frame.height + cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({0.0, -_frame.height + cornerRadius.y});
					segment.controlPoints.push_back({0.0, -cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({0.0, -cornerRadius.y});
					segment.controlPoints.push_back({0.0, 0.0});
					segment.controlPoints.push_back({cornerRadius.x, 0.0});
					mypath.segments.push_back(segment);
						
					paths.paths.push_back(mypath);
					mypath.segments.clear();
					
					
					//Inner outline
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({cornerRadius.x, -_outlineThickness});
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, -_outlineThickness});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, -_outlineThickness});
					segment.controlPoints.push_back({_frame.width - _outlineThickness, -_outlineThickness});
					segment.controlPoints.push_back({_frame.width - _outlineThickness, -cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width - _outlineThickness, -cornerRadius.y});
					segment.controlPoints.push_back({_frame.width - _outlineThickness, -_frame.height + cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width - _outlineThickness, -_frame.height + cornerRadius.y});
					segment.controlPoints.push_back({_frame.width - _outlineThickness, -_frame.height + _outlineThickness});
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, -_frame.height + _outlineThickness});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_frame.width - cornerRadius.x, -_frame.height + _outlineThickness});
					segment.controlPoints.push_back({cornerRadius.x, -_frame.height + _outlineThickness});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({cornerRadius.x, -_frame.height + _outlineThickness});
					segment.controlPoints.push_back({_outlineThickness, -_frame.height + _outlineThickness});
					segment.controlPoints.push_back({_outlineThickness, -_frame.height + cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeLine;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_outlineThickness, -_frame.height + cornerRadius.y});
					segment.controlPoints.push_back({_outlineThickness, -cornerRadius.y});
					mypath.segments.push_back(segment);
					
					segment.type = KG::PathSegment::TypeBezierQuadratic;
					segment.controlPoints.clear();
					segment.controlPoints.push_back({_outlineThickness, -cornerRadius.y});
					segment.controlPoints.push_back({_outlineThickness, -_outlineThickness});
					segment.controlPoints.push_back({cornerRadius.x, -_outlineThickness});
					mypath.segments.push_back(segment);
						
					paths.paths.push_back(mypath);
					
					outlineTriangleMesh = KG::MeshGeneratorLoopBlinn::GetMeshForPathCollection(paths);
					outlineTriangleMeshVertexFloatCount = 5;

					vertexCount += outlineTriangleMesh.vertices.size() / outlineTriangleMeshVertexFloatCount;
					indexCount += outlineTriangleMesh.indices.size();
				}
				
				float *vertexPositionBuffer = new float[vertexCount * vertexPositionSize];
				float *vertexUV0Buffer = new float[vertexCount * 2];
				float *vertexUV1Buffer = new float[vertexCount * 3];
				uint32 *indexBuffer = new uint32[indexCount];
				
				vertexPositionBuffer[0 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[0 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[0 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[1 * vertexPositionSize + 0] = cornerRadius.x;
				vertexPositionBuffer[1 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[1 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[2 * vertexPositionSize + 0] = cornerRadius.x;
				vertexPositionBuffer[2 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[2 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[3 * vertexPositionSize + 0] = _frame.width - cornerRadius.y;
				vertexPositionBuffer[3 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[3 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[4 * vertexPositionSize + 0] = _frame.width - cornerRadius.y;
				vertexPositionBuffer[4 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[4 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[5 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[5 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[5 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[6 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[6 * vertexPositionSize + 1] = -cornerRadius.y;
				if(_hasOutline) vertexPositionBuffer[6 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[7 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[7 * vertexPositionSize + 1] = -cornerRadius.y;
				if(_hasOutline) vertexPositionBuffer[7 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[8 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[8 * vertexPositionSize + 1] = cornerRadius.w - _frame.height;
				if(_hasOutline) vertexPositionBuffer[8 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[9 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[9 * vertexPositionSize + 1] = cornerRadius.w - _frame.height;
				if(_hasOutline) vertexPositionBuffer[9 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[10 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[10 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[10 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[11 * vertexPositionSize + 0] = _frame.width - cornerRadius.w;
				vertexPositionBuffer[11 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[11 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[12 * vertexPositionSize + 0] = _frame.width - cornerRadius.w;
				vertexPositionBuffer[12 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[12 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[13 * vertexPositionSize + 0] = cornerRadius.z;
				vertexPositionBuffer[13 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[13 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[14 * vertexPositionSize + 0] = cornerRadius.z;
				vertexPositionBuffer[14 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[14 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[15 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[15 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[15 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[16 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[16 * vertexPositionSize + 1] = cornerRadius.z - _frame.height;
				if(_hasOutline) vertexPositionBuffer[16 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[17 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[17 * vertexPositionSize + 1] = cornerRadius.z - _frame.height;
				if(_hasOutline) vertexPositionBuffer[17 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[18 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[18 * vertexPositionSize + 1] = -cornerRadius.x;
				if(_hasOutline) vertexPositionBuffer[18 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[19 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[19 * vertexPositionSize + 1] = -cornerRadius.x;
				if(_hasOutline) vertexPositionBuffer[19 * vertexPositionSize + 2] = 0.0f;
				
				for(int i = 0; i < 20; i++)
				{
					vertexUV0Buffer[i * 2 + 0] = vertexPositionBuffer[i * vertexPositionSize + 0] / _frame.width;
					vertexUV0Buffer[i * 2 + 1] = -vertexPositionBuffer[i * vertexPositionSize + 1] / _frame.height;
					
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
				
				if(_hasOutline && _outlineThickness > RN::k::EpsilonFloat)
				{
					for(size_t i = 0; i < outlineTriangleMesh.vertices.size() / outlineTriangleMeshVertexFloatCount; i++)
					{
						vertexPositionBuffer[(i + 20) * vertexPositionSize + 0] = outlineTriangleMesh.vertices[i * outlineTriangleMeshVertexFloatCount + 0];
						vertexPositionBuffer[(i + 20) * vertexPositionSize + 1] = outlineTriangleMesh.vertices[i * outlineTriangleMeshVertexFloatCount + 1];
						vertexPositionBuffer[(i + 20) * vertexPositionSize + 2] = 1.0f;
						
						vertexUV0Buffer[(i + 20) * 2 + 0] = vertexPositionBuffer[(i + 20) * vertexPositionSize + 0] / _frame.width;
						vertexUV0Buffer[(i + 20) * 2 + 1] = -vertexPositionBuffer[(i + 20) * vertexPositionSize + 1] / _frame.height;
						
						vertexUV1Buffer[(i + 20) * 3 + 0] = outlineTriangleMesh.vertices[i * outlineTriangleMeshVertexFloatCount + 2];
						vertexUV1Buffer[(i + 20) * 3 + 1] = outlineTriangleMesh.vertices[i * outlineTriangleMeshVertexFloatCount + 3];
						vertexUV1Buffer[(i + 20) * 3 + 2] = outlineTriangleMesh.vertices[i * outlineTriangleMeshVertexFloatCount + 4];
					}
					
					for(size_t i = 0; i < outlineTriangleMesh.indices.size(); i++)
					{
						indexBuffer[(i + 30)] = 20 + outlineTriangleMesh.indices[i];
					}
				}
				
				std::vector<Mesh::VertexAttribute> meshVertexAttributes;
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, _hasOutline? PrimitiveType::Vector3 : PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector3);
				
				mesh = new Mesh(meshVertexAttributes, vertexCount, indexCount);
				mesh->BeginChanges();
				
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUV0Buffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords1, vertexUV1Buffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
				
				mesh->EndChanges();
				
				delete[] vertexPositionBuffer;
				delete[] vertexUV0Buffer;
				delete[] vertexUV1Buffer;
				delete[] indexBuffer;
			}
			else
			{
				size_t vertexCount = _hasOutline? 12 : 4;
				size_t indexCount = _hasOutline? 30 : 6;
				size_t vertexPositionSize = (_hasOutline? 3 : 2);
				float *vertexPositionBuffer = new float[vertexCount * vertexPositionSize];
				float *vertexUVBuffer = new float[vertexCount * 2];
				uint32 *indexBuffer = new uint32[indexCount];
				
				vertexPositionBuffer[0 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[0 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[0 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[1 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[1 * vertexPositionSize + 1] = -_outlineThickness;
				if(_hasOutline) vertexPositionBuffer[1 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[2 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
				vertexPositionBuffer[2 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[2 * vertexPositionSize + 2] = 0.0f;
				
				vertexPositionBuffer[3 * vertexPositionSize + 0] = _outlineThickness;
				vertexPositionBuffer[3 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
				if(_hasOutline) vertexPositionBuffer[3 * vertexPositionSize + 2] = 0.0f;
				
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
				
				if(_hasOutline)
				{
					//Inner vertices of outline
					vertexPositionBuffer[4 * vertexPositionSize + 0] = _outlineThickness;
					vertexPositionBuffer[4 * vertexPositionSize + 1] = -_outlineThickness;
					vertexPositionBuffer[4 * vertexPositionSize + 2] = 1.0f;
					
					vertexPositionBuffer[5 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
					vertexPositionBuffer[5 * vertexPositionSize + 1] = -_outlineThickness;
					vertexPositionBuffer[5 * vertexPositionSize + 2] = 1.0f;
					
					vertexPositionBuffer[6 * vertexPositionSize + 0] = _frame.width - _outlineThickness;
					vertexPositionBuffer[6 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
					vertexPositionBuffer[6 * vertexPositionSize + 2] = 1.0f;
					
					vertexPositionBuffer[7 * vertexPositionSize + 0] = _outlineThickness;
					vertexPositionBuffer[7 * vertexPositionSize + 1] = -_frame.height + _outlineThickness;
					vertexPositionBuffer[7 * vertexPositionSize + 2] = 1.0f;
					
					vertexUVBuffer[4 * 2 + 0] = 0.0f;
					vertexUVBuffer[4 * 2 + 1] = 0.0f;
					
					vertexUVBuffer[5 * 2 + 0] = 1.0f;
					vertexUVBuffer[5 * 2 + 1] = 0.0f;
					
					vertexUVBuffer[6 * 2 + 0] = 1.0f;
					vertexUVBuffer[6 * 2 + 1] = 1.0f;
					
					vertexUVBuffer[7 * 2 + 0] = 0.0f;
					vertexUVBuffer[7 * 2 + 1] = 1.0f;
					
					//Outter vertices of outline
					vertexPositionBuffer[8 * vertexPositionSize + 0] = 0.0f;
					vertexPositionBuffer[8 * vertexPositionSize + 1] = 0.0f;
					vertexPositionBuffer[8 * vertexPositionSize + 2] = 1.0f;
					
					vertexPositionBuffer[9 * vertexPositionSize + 0] = _frame.width;
					vertexPositionBuffer[9 * vertexPositionSize + 1] = 0.0f;
					vertexPositionBuffer[9 * vertexPositionSize + 2] = 1.0f;
					
					vertexPositionBuffer[10 * vertexPositionSize + 0] = _frame.width;
					vertexPositionBuffer[10 * vertexPositionSize + 1] = -_frame.height;
					vertexPositionBuffer[10 * vertexPositionSize + 2] = 1.0f;
					
					vertexPositionBuffer[11 * vertexPositionSize + 0] = 0.0f;
					vertexPositionBuffer[11 * vertexPositionSize + 1] = -_frame.height;
					vertexPositionBuffer[11 * vertexPositionSize + 2] = 1.0f;
					
					vertexUVBuffer[8 * 2 + 0] = 0.0f;
					vertexUVBuffer[8 * 2 + 1] = 0.0f;
					
					vertexUVBuffer[9 * 2 + 0] = 1.0f;
					vertexUVBuffer[9 * 2 + 1] = 0.0f;
					
					vertexUVBuffer[10 * 2 + 0] = 1.0f;
					vertexUVBuffer[10 * 2 + 1] = 1.0f;
					
					vertexUVBuffer[11 * 2 + 0] = 0.0f;
					vertexUVBuffer[11 * 2 + 1] = 1.0f;
					
					//Top part of the outline
					indexBuffer[6] = 8; //top left
					indexBuffer[7] = 4; //bottom left
					indexBuffer[8] = 9; //top right
					
					indexBuffer[9] = 4; //bottom left
					indexBuffer[10] = 5; //bottom right
					indexBuffer[11] = 9; //top right
					
					//Bottom part of the outline
					indexBuffer[12] = 7; //top left
					indexBuffer[13] = 11; //bottom left
					indexBuffer[14] = 6; //top right
					
					indexBuffer[15] = 11; //bottom left
					indexBuffer[16] = 10; //bottom right
					indexBuffer[17] = 6; //top right
					
					//Left part of the outline
					indexBuffer[18] = 8; //top left
					indexBuffer[19] = 11; //bottom left
					indexBuffer[20] = 4; //top right
					
					indexBuffer[21] = 11; //bottom left
					indexBuffer[22] = 7; //bottom right
					indexBuffer[23] = 4; //top right
					
					//Right part of the outline
					indexBuffer[24] = 5; //top left
					indexBuffer[25] = 6; //bottom left
					indexBuffer[26] = 9; //top right
					
					indexBuffer[27] = 6; //bottom left
					indexBuffer[28] = 10; //bottom right
					indexBuffer[29] = 9; //top right
				}
				
				std::vector<Mesh::VertexAttribute> meshVertexAttributes;
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, _hasOutline? PrimitiveType::Vector3 : PrimitiveType::Vector2);
				meshVertexAttributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
				
				mesh = new Mesh(meshVertexAttributes, vertexCount, indexCount);
				mesh->BeginChanges();
				
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, vertexUVBuffer);
				mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indexBuffer);
				
				mesh->EndChanges();
				
				delete[] vertexPositionBuffer;
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
				if(_hasBackgroundGradient) shaderOptions->AddDefine("RN_UI_GRADIENT", "1");
				if(_isCircle) shaderOptions->AddDefine("RN_UI_CIRCLE", "1");
				if(_hasOutline) shaderOptions->AddDefine("RN_UI_OUTLINE", "1");
				material->SetAlphaToCoverage(false);
				material->SetCullMode(CullMode::None);
				material->SetDepthMode(_depthMode);
				material->SetDepthWriteEnabled(_isDepthWriteEnabled);
				material->SetColorWriteMask(_isColorWriteEnabled, _isColorWriteEnabled, _isColorWriteEnabled, _isAlphaWriteEnabled);
				material->SetPolygonOffset(_isDepthWriteEnabled, _depthFactor, _depthOffset);
				material->SetBlendFactorSource(_blendSourceFactorRGB, _blendSourceFactorA);
				material->SetBlendFactorDestination(_blendDestinationFactorRGB, _blendDestinationFactorA);
				material->SetBlendOperation(_blendOperationRGB, _blendOperationA);
				if(!_hasBackgroundGradient)
				{
					Color finalColor = _backgroundColor[0];
					finalColor.a *= _combinedOpacityFactor;
					material->SetSkipRendering(finalColor.a < k::EpsilonFloat);
					material->SetDiffuseColor(finalColor);
				}
				else
				{
					Color finalColor[4];
					finalColor[0] = _backgroundColor[0];
					finalColor[0].a *= _combinedOpacityFactor;
					finalColor[1] = _backgroundColor[1];
					finalColor[1].a *= _combinedOpacityFactor;
					finalColor[2] = _backgroundColor[2];
					finalColor[2].a *= _combinedOpacityFactor;
					finalColor[3] = _backgroundColor[3];
					finalColor[3].a *= _combinedOpacityFactor;
					
					material->SetDiffuseColor(finalColor[0]);
					material->SetSpecularColor(finalColor[1]);
					material->SetEmissiveColor(finalColor[2]);
					material->SetAmbientColor(finalColor[3]);
					
					material->SetSkipRendering(finalColor[0].a + finalColor[1].a + finalColor[2].a + finalColor[3].a < k::EpsilonFloat);
				}
				
				if(_hasOutline)
				{
					Color finalColor;
					finalColor = _outlineColor;
					finalColor.a *= _combinedOpacityFactor;
					
					material->SetUIOutlineColor(finalColor);
					
					//TODO: Skip rendering stuff should also depend on the outline being visible or not... see background gradient...
				}

				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions));
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions));
				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);

				material->SetUIClippingRect(Vector4(_scissorRect.GetLeft(), _scissorRect.GetRight(), _scissorRect.GetTop(), _scissorRect.GetBottom()));
				material->SetUIOffset(Vector2(0.0f, 0.0f));

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
			if(_isHidden || isParentHidden || !_bounds.IntersectsRect(_scissorRect) || _combinedOpacityFactor <= k::EpsilonFloat)
			{
				AddFlags(SceneNode::Flags::Hidden);
			}
			else
			{
				RemoveFlags(SceneNode::Flags::Hidden);
			}
			
			if(_needsMeshUpdate && !_isHidden && !isParentHidden && _combinedOpacityFactor > k::EpsilonFloat)
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
