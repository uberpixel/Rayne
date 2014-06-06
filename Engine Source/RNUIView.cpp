//
//  RNView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIView.h"
#include "RNUIWidget.h"
#include "RNUIServer.h"
#include "RNUIStyle.h"
#include "RNResourceCoordinator.h"
#include "RNDebug.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(View, Responder)
		
		View::View()
		{
			Initialize();
		}
		
		View::View(const Rect &frame) :
			_frame(frame),
			_bounds(0.0f, 0.0f, frame.width, frame.height)
		{
			Initialize();
		}
		
		View::~View()
		{
			_material->Release();
			
			Widget *widget = GetWidget();
			
			if(widget && widget->GetFirstResponder() == this)
				widget->ForceResignFirstResponder();
		}
		
		// ---------------------
		// MARK: -
		// MARK: Helper
		// ---------------------
		
		void View::Initialize()
		{
			_superview    = nullptr;
			_widget       = nullptr;
			_clippingView = nullptr;
			_mesh         = nullptr;
			
			_dirtyLayout        = true;
			_interactionEnabled = true;
			_clipSubviews       = false;
			_clipWithWidget     = true;
			_hidden             = false;
			_autoresizingMask   = 0;
			
			_material = GetBasicMaterial(Shader::WithFile("shader/rn_UIView"));
			_material->Retain();
			
			SetBackgroundColor(Style::GetSharedInstance()->GetColor(Style::ColorStyle::BackgroundColor)->GetRNColor());
		}
		
		void View::SetBackgroundColor(const RN::Color& color)
		{
			_material->SetDiffuseColor(color);
		}
		
		Mesh *View::GetBasicMesh(const Vector2 &size)
		{
			MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			
			MeshDescriptor uvDescriptor(MeshFeature::UVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			Mesh *mesh = new Mesh(descriptors, 4, 0);
			mesh->SetDrawMode(Mesh::DrawMode::TriangleStrip);
			
			Mesh::Chunk chunk = mesh->GetChunk();
			
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(MeshFeature::Vertices);
			Mesh::ElementIterator<Vector2> uvCoords = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
			
			*vertices ++ = Vector2(size.x, size.y);
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(size.x, 0.0f);
			*vertices ++ = Vector2(0.0f, 0.0f);
			
			*uvCoords ++ = Vector2(1.0f, 0.0f);
			*uvCoords ++ = Vector2(0.0f, 0.0f);
			*uvCoords ++ = Vector2(1.0f, 1.0f);
			*uvCoords ++ = Vector2(0.0f, 1.0f);
			
			chunk.CommitChanges();
			
			return mesh->Autorelease();
		}
		
		Material *View::GetBasicMaterial(Shader *shader)
		{
			Material *material = new Material(shader);
			material->SetDepthTest(false);
			material->SetDepthWrite(false);
			material->SetBlending(true);
			material->SetBlendMode(Material::BlendMode::SourceAlpha, Material::BlendMode::OneMinusSourceAlpha);
			material->SetAlphaBlendEquation(Material::BlendEquation::Max);
			material->SetLighting(false);
			
			return material->Autorelease();
		}
		
		void View::UpdateBasicMesh(Mesh *mesh, const Vector2 &size)
		{
			Mesh::Chunk chunk = mesh->GetChunk();
			Mesh::ElementIterator<Vector2> vertices = chunk.GetIterator<Vector2>(MeshFeature::Vertices);
			
			*vertices ++ = Vector2(size.x, size.y);
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(size.x, 0.0f);
			*vertices ++ = Vector2(0.0f, 0.0f);
			
			chunk.CommitChanges();
		}
		
		Responder *View::GetNextResponder() const
		{
			if(_superview)
				return _superview;
			
			return _widget;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Coordinate systems
		// ---------------------
		
		void View::ConvertPointToWidget(Vector2& point)
		{
			View *view = _superview;
			while(view)
			{
				point.x += view->_frame.x - view->_bounds.x;
				point.y += view->_frame.y - view->_bounds.y;
				
				view = view->_superview;
			}
			
			point.x += _frame.x - _bounds.x;
			point.y += _frame.y - _bounds.y;
		}
		
		void View::ConvertPointFromWidget(Vector2& point)
		{
			View *view = _superview;
			while(view)
			{
				point.x -= view->_frame.x - view->_bounds.x;
				point.y -= view->_frame.y - view->_bounds.y;
				
				view = view->_superview;
			}
			
			point.x -= _frame.x - _bounds.x;
			point.y -= _frame.y - _bounds.y;
		}
		
		Vector2 View::ConvertPointToView(const Vector2 &point, View *view)
		{
			Vector2 converted = point;
			ConvertPointToWidget(converted);
			
			if(!view)
				return converted;
			
			view->ConvertPointFromWidget(converted);
			return converted;
		}
		
		Vector2 View::ConvertPointFromView(const Vector2 &point, View *view)
		{
			Vector2 converted = point;
			
			if(view)
				view->ConvertPointToWidget(converted);
			
			ConvertPointFromWidget(converted);
			return converted;
		}
		
		Vector2 View::ConvertPointToBase(const Vector2 &point)
		{
			Vector2 converted = point;
			ConvertPointToWidget(converted);
			
			if(_widget)
			{
				converted.x += _widget->_frame.x;
				converted.y += _widget->_frame.y;
			}
			
			return converted;
		}
		
		Vector2 View::ConvertPointFromBase(const Vector2 &point)
		{
			Vector2 converted = point;
			if(_widget)
			{
				converted.x -= _widget->_frame.x;
				converted.y -= _widget->_frame.y;
			}
			
			ConvertPointFromWidget(converted);
			return converted;
		}
		
		
		Rect View::ConvertRectToView(const Rect &frame, View *view)
		{
			Rect converted = frame;
			Vector2 point  = ConvertPointToView(Vector2(frame.x, frame.y), view);
			
			converted.x = point.x;
			converted.y = point.y;
			
			return converted;
		}
		
		Rect View::ConvertRectFromView(const Rect &frame, View *view)
		{
			Rect converted = frame;
			Vector2 point  = ConvertPointFromView(Vector2(frame.x, frame.y), view);
			
			converted.x = point.x;
			converted.y = point.y;
			
			return converted;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Hit test
		// ---------------------
		
		View *View::HitTest(const Vector2 &tpoint, Event *event)
		{
			bool traverse = true;
			View *potential = this;
			Vector2 point = tpoint;
			
			//TODO: This line causes problems if the widgets content view has bounds.xy other than 0
			//		Things might break without if the widgets content views frame.xy is not equal 0
			//		or if called on a view directly, independent of its widget.
		//	ConvertPointToWidget(point);
			
			while(traverse)
			{
				traverse = false;
				
				size_t count = potential->_subviews.GetCount();
				
				for(size_t i = 0; i < count; i++)
				{
					View *view = potential->_subviews.GetObjectAtIndex<View>(count - i - 1);
					Vector2 transformed = std::move(view->ConvertPointFromView(point, nullptr));
					
					if(!view->_hidden && view->_interactionEnabled && view->IsPointInside(transformed, event))
					{
						potential = view;
						traverse = true;
						break;
					}
				}
			}
			
			return (potential && potential->_interactionEnabled) ? potential : nullptr;
		}
		
		bool View::IsPointInside(const Vector2 &point, Event *event)
		{
			return _bounds.ContainsPoint(point);
		}
		
		void View::SetInteractionEnabled(bool enabled)
		{
			_interactionEnabled = enabled;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Subviews
		// ---------------------
		
		void View::SetClipSubviews(bool clipping)
		{
			if(_clipSubviews == clipping)
				return;
			
			_clipSubviews = clipping;
			SetNeedsLayoutUpdate();
		}
		
		void View::SetClipInsets(const EdgeInsets &insets)
		{
			_clipInsets = insets;
			SetNeedsLayoutUpdate();
		}
		
		void View::ViewHierarchyChanged()
		{
			size_t count = _subviews.GetCount();
			
			for(size_t i = 0; i < count; i++)
			{
				View *subview = _subviews.GetObjectAtIndex<View>(i);
				subview->_widget = _widget;
				subview->_clipWithWidget = _clipWithWidget;
				subview->ViewHierarchyChanged();
			}
			
			_dirtyLayout = true;
		}
		
		void View::AddSubview(View *subview)
		{
			subview->Retain();
			
			if(subview->_superview)
				subview->RemoveFromSuperview();
			
			subview->WillMoveToSuperview(this);
			
			_subviews.AddObject(subview);
			
			subview->_superview = this;
			subview->_widget = _widget;
			
			subview->ViewHierarchyChanged();
			subview->DidMoveToSuperview(this);
			subview->Release();
			
			DidAddSubview(subview);
			SetNeedsLayoutUpdate();
		}
		
		void View::RemoveSubview(View *subview)
		{
			size_t index = _subviews.GetIndexOfObject(subview);
			if(index != kRNNotFound)
			{
				WillRemoveSubview(subview);
				
				subview->Retain();
				subview->WillMoveToSuperview(nullptr);
				
				_subviews.RemoveObjectAtIndex(index);
				
				subview->_superview = 0;
				subview->_widget = 0;
				
				subview->DidMoveToSuperview(nullptr);
				subview->Release();
				
				SetNeedsLayoutUpdate();
			}
		}
		
		void View::RemoveAllSubviews()
		{
			size_t count = _subviews.GetCount();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews.GetObjectAtIndex<View>(i);
				
				WillRemoveSubview(subview);
				subview->WillMoveToSuperview(nullptr);
				
				subview->_superview = 0;
				subview->_widget = 0;
				
				subview->DidMoveToSuperview(nullptr);
			}
			
			_subviews.RemoveAllObjects();
			SetNeedsLayoutUpdate();
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
				
				_subviews.RemoveObject(subview);
				_subviews.AddObject(subview);
				
				subview->Release();
				DidBringSubviewToFront(subview);
			}
		}
		
		void View::SendSubviewToBack(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();
				
				if(_subviews.GetCount() > 1)
				{
					_subviews.RemoveObject(subview);
					_subviews.InsertObjectAtIndex(subview, 0);
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
		// MARK: Layout engine
		// ---------------------
		
		void View::SizeToFit()
		{
			Vector2 size = std::move(GetSizeThatFits());
			Rect frame = _frame;
			
			frame.width  = size.x;
			frame.height = size.y;
			
			SetFrame(frame);
			SetNeedsLayoutUpdate();
		}
		
		Vector2 View::GetSizeThatFits()
		{
			return _frame.GetSize();
		}
		
		void View::SetFrame(const Rect &frame)
		{
			Vector2 oldSize = _frame.GetSize();
			
			_frame = frame;
			
			_bounds.width  = frame.width;
			_bounds.height = frame.height;
			
			SetNeedsLayoutUpdate();
			ResizeSubviewsFromOldSize(oldSize);
		}
		
		void View::SetBounds(const Rect &bounds)
		{
			if(_frame.GetSize() != bounds.GetSize())
			{
				Vector2 size = _frame.GetSize();
				
				_frame.width  = bounds.width;
				_frame.height = bounds.height;
				
				ResizeSubviewsFromOldSize(size);
			}
			
			_bounds = bounds;
			
			SetNeedsLayoutUpdate();
		}
		
		void View::SetNeedsLayoutUpdate()
		{
			_dirtyLayout = true;
			_subviews.Enumerate<View>([&](View *subview, size_t index, bool &stop) {
				subview->SetNeedsLayoutUpdate();
			});
		}
		
		void View::SetHidden(bool hidden)
		{
			_hidden = hidden;
		}
		
		void View::SetAutoresizingMask(AutoresizingMask mask)
		{
			_autoresizingMask = mask;
		}
		
		void View::SetTransform(const Matrix &transform)
		{
			_transform = transform;
			SetNeedsLayoutUpdate();
		}
		
		void View::ResizeSubviewsFromOldSize(const Vector2 &oldSize)
		{
			Vector2 size = _frame.GetSize();
			Vector2 diff = size - oldSize;
			
			_subviews.Enumerate<View>([&](View *subview, size_t index, bool &stop) {
				
				if(subview->_autoresizingMask == 0)
					return;
				
				Rect frame = subview->GetFrame();
				
				uint32 stepsWidth  = 0;
				uint32 stepsHeight = 0;
				
				
				if(subview->_autoresizingMask & AutoresizingMask::FlexibleLeftMargin)
					stepsWidth ++;
				
				if(subview->_autoresizingMask & AutoresizingMask::FlexibleRightMargin)
					stepsWidth ++;
				
				if(subview->_autoresizingMask & AutoresizingMask::FlexibleWidth)
					stepsWidth ++;
				
				
				if(subview->_autoresizingMask & AutoresizingMask::FlexibleTopMargin)
					stepsHeight ++;
				
				if(subview->_autoresizingMask & AutoresizingMask::FlexibleBottomMargin)
					stepsHeight ++;
				
				if(subview->_autoresizingMask & AutoresizingMask::FlexibleHeight)
					stepsHeight ++;
				
				
				
				if(stepsWidth > 0)
				{
					float distribution = diff.x / stepsWidth;
					
					if(subview->_autoresizingMask & AutoresizingMask::FlexibleLeftMargin)
						frame.x += distribution;
					
					if(subview->_autoresizingMask & AutoresizingMask::FlexibleWidth)
						frame.width += distribution;
				}
				
				
				if(stepsHeight > 0)
				{
					float distribution = diff.y / stepsHeight;
					
					if(subview->_autoresizingMask & AutoresizingMask::FlexibleTopMargin)
						frame.y += distribution;
					
					if(subview->_autoresizingMask & AutoresizingMask::FlexibleHeight)
						frame.height += distribution;
				}
				
				subview->SetFrame(frame);
			});
		}
		
		// ---------------------
		// MARK: -
		// MARK: Rendering
		// ---------------------
		
		void View::CalculateScissorRect()
		{
			float serverHeight = (_widget && _widget->_server) ? _widget->_server->GetHeight() : 0.0f;
			Vector2 origin = _frame.GetOrigin();
			
			_clippingView = nullptr;
			
			View *view = _superview;
			while(view)
			{
				if(!_clippingView && view->_clipSubviews)
					_clippingView = view;
				
				origin.x += view->_frame.x - view->_bounds.x;
				origin.y += view->_frame.y - view->_bounds.y;
				
				view = view->_superview;
			}
			
			if(_widget)
			{
				origin.x += _widget->_frame.x;
				origin.y += _widget->_frame.y;
			}
			
			_scissorRect.x = origin.x;
			_scissorRect.y = serverHeight - _frame.height - origin.y;
			_scissorRect.width  = _frame.width;
			_scissorRect.height = _frame.height;
			
			_scissorRect.x     += _clipInsets.left;
			_scissorRect.width -= _clipInsets.left + _clipInsets.right;
			
			_scissorRect.y      += _clipInsets.bottom;
			_scissorRect.height -= _clipInsets.bottom + _clipInsets.top;
			
			if(_clippingView)
			{
				_scissorRect.x     = std::max(_scissorRect.x, _clippingView->_scissorRect.x);
				_scissorRect.width = std::min(_scissorRect.width, _clippingView->_scissorRect.GetRight() - _scissorRect.x);
				
				_scissorRect.y      = std::max(_scissorRect.y, _clippingView->_scissorRect.y);
				_scissorRect.height = std::min(_scissorRect.height, _clippingView->_scissorRect.GetBottom() - _scissorRect.y);
			}
		}
		
		void View::LayoutSubviews()
		{}
		
		void View::LayoutIfNeeded()
		{
			if(_dirtyLayout)
			{
				Vector2 converted  = ConvertPointToView(_bounds.GetOrigin(), nullptr);
				float serverHeight = 0.0f;
				
				if(_superview)
				{
					_intermediateTransform = _superview->_intermediateTransform * _transform;
				}
				else if(_widget)
				{
					_intermediateTransform = _widget->_transform * _transform;
				}
				
				if(_widget)
				{
					converted.x += _widget->_frame.x;
					converted.y += _widget->_frame.y;
					
					if(_widget->_server)
						serverHeight = _widget->_server->GetHeight();
				}
				
				_finalTransform = _intermediateTransform;
				_finalTransform.Translate(Vector3(converted.x, serverHeight - _frame.height - converted.y, 0.0f));
				
				if(_mesh)
					UpdateBasicMesh(_mesh, _frame.GetSize());
				else
					_mesh = GetBasicMesh(_frame.GetSize())->Retain();
				
				CalculateScissorRect();
				LayoutSubviews();
				
				_dirtyLayout = false;
			}
		}
		
		void View::Update()
		{
			LayoutIfNeeded();
			
			if(_widget && _widget->_server && _widget->_server->GetDrawDebugFrames())
			{
				Rect frame = ConvertRectToView(GetBounds(), nullptr);
				
				if(_widget)
				{
					frame.x += _widget->_frame.x;
					frame.y += _widget->_frame.y;
				}
				
				Debug::AddLinePoint(Vector2(frame.GetLeft(), frame.GetBottom()), RN::Color::Red());
				Debug::AddLinePoint(Vector2(frame.GetRight(), frame.GetBottom()), RN::Color::Red());
				Debug::AddLinePoint(Vector2(frame.GetRight(), frame.GetTop()), RN::Color::Red());
				Debug::AddLinePoint(Vector2(frame.GetLeft(), frame.GetTop()), RN::Color::Red());
				Debug::AddLinePoint(Vector2(frame.GetLeft(), frame.GetBottom()), RN::Color::Red());
				Debug::EndLine();
			}
		}
		
		void View::PopulateRenderingObject(RenderingObject& object)
		{
			object.material = _material;
			object.transform = &_finalTransform;
			object.flags |= RenderingObject::ScissorTest;
			
			if(_clippingView)
			{
				object.scissorRect = _clippingView->_scissorRect;
			}
			else if(_widget && _clipWithWidget)
			{
				float serverHeight = (_widget->_server) ? _widget->_server->GetHeight() : 0.0f;
				
				object.scissorRect = _widget->GetFrame();
				object.scissorRect.y = serverHeight - object.scissorRect.height - object.scissorRect.y;
			}
			else if(!_clipWithWidget)
			{
				object.flags &= ~RenderingObject::ScissorTest;
			}
		}
		
		
		void View::Draw(Renderer *renderer)
		{
			if(_material->GetDiffuseColor().a >= k::EpsilonFloat)
			{
				RenderingObject object;
				PopulateRenderingObject(object);
				
				object.mesh = _mesh;
				object.material = _material;
				
				renderer->RenderObject(object);
			}
		}
		
		
		
		void View::UpdateRecursively()
		{
			Update();
			
			size_t count = _subviews.GetCount();
			
			for(size_t i = 0; i < count; i ++)
			{
				View *subview = _subviews.GetObjectAtIndex<View>(i);
				subview->UpdateRecursively();
			}
		}
		
		void View::DrawRecursively(Renderer *renderer)
		{
			if(_hidden)
				return;
			
			Draw(renderer);
			
			size_t count = _subviews.GetCount();
			
			for(size_t i = 0; i < count; i ++)
			{
				View *subview = _subviews.GetObjectAtIndex<View>(i);
				subview->DrawRecursively(renderer);
			}
		}
	}
}
