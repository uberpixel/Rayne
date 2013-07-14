//
//  RNView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIView.h"
#include "RNUIWidget.h"
#include "RNUIServer.h"
#include "RNResourcePool.h"
#include "RNDebug.h"

#define kRNViewShaderResourceName "kRNViewShaderResourceName"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(View)
		
		View::View()
		{
			Initialize();
		}
		
		View::View(const Rect& frame) :
			_frame(frame),
			_bounds(0.0f, 0.0f, frame.width, frame.height)
		{
			Initialize();
		}
		
		View::~View()
		{
			_material->Release();
		}
		
		// ---------------------
		// MARK: -
		// MARK: Helper
		// ---------------------
		
		void View::Initialize()
		{
			static std::once_flag flag;
			std::call_once(flag, []() {
				Shader *shader = new Shader("shader/rn_View");
				ResourcePool::SharedInstance()->AddResource(shader, kRNViewShaderResourceName);
			});
			
			_superview    = nullptr;
			_widget       = nullptr;
			_clippingView = nullptr;
			
			_dirtyLayout        = true;
			_interactionEnabled = true;
			_clipSubviews       = false;
			
			_material = new Material(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNViewShaderResourceName));
			_material->depthtest  = false;
			_material->depthwrite = false;
			_material->blending   = true;
			_material->lighting   = false;
			_material->diffuse    = Color(1.0f, 1.0f, 1.0f, 1.0f);
		}
		
		void View::SetBackgroundColor(const Color& color)
		{
			_material->diffuse = color;
		}
		
		Mesh *View::BasicMesh(const Vector2& size)
		{
			MeshDescriptor vertexDescriptor(kMeshFeatureVertices);
			vertexDescriptor.elementMember = 2;
			vertexDescriptor.elementSize   = sizeof(Vector2);
			vertexDescriptor.elementCount  = 4;
			
			MeshDescriptor uvDescriptor(kMeshFeatureUVSet0);
			uvDescriptor.elementMember = 2;
			uvDescriptor.elementSize   = sizeof(Vector2);
			uvDescriptor.elementCount  = 4;
			
			std::vector<MeshDescriptor> descriptors = { vertexDescriptor, uvDescriptor };
			Mesh *mesh = new Mesh(descriptors);
			mesh->SetMode(GL_TRIANGLE_STRIP);
			
			Vector2 *vertices = mesh->Element<Vector2>(kMeshFeatureVertices);
			Vector2 *uvCoords = mesh->Element<Vector2>(kMeshFeatureUVSet0);
			
			*vertices ++ = Vector2(size.x, size.y);
			*vertices ++ = Vector2(0.0f, size.y);
			*vertices ++ = Vector2(size.x, 0.0f);
			*vertices ++ = Vector2(0.0f, 0.0f);
			
			*uvCoords ++ = Vector2(1.0f, 0.0f);
			*uvCoords ++ = Vector2(0.0f, 0.0f);
			*uvCoords ++ = Vector2(1.0f, 1.0f);
			*uvCoords ++ = Vector2(0.0f, 1.0f);
			
			mesh->ReleaseElement(kMeshFeatureVertices);
			mesh->ReleaseElement(kMeshFeatureUVSet0);
			mesh->UpdateMesh();
			
			return mesh->Autorelease();
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
		
		Vector2 View::ConvertPointToView(const Vector2& point, View *view)
		{
			Vector2 converted = point;
			ConvertPointToWidget(converted);
			
			if(!view)
				return converted;
			
			view->ConvertPointFromWidget(converted);
			return converted;
		}
		
		Vector2 View::ConvertPointFromView(const Vector2& point, View *view)
		{
			Vector2 converted = point;
			
			if(view)
				view->ConvertPointToWidget(converted);
			
			ConvertPointFromWidget(converted);
			return converted;
		}
		
		
		Rect View::ConvertRectToView(const Rect& frame, View *view)
		{
			Rect converted = frame;
			Vector2 point  = ConvertPointToView(Vector2(frame.x, frame.y), view);
			
			converted.x = point.x;
			converted.y = point.y;
			
			return converted;
		}
		
		Rect View::ConvertRectFromView(const Rect& frame, View *view)
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
		
		View *View::HitTest(const Vector2& tpoint, Event *event)
		{
			bool traverse = true;
			View *potential = this;
			Vector2 point = ConvertPointToView(tpoint, nullptr);
			
			while(traverse)
			{
				traverse = false;
				
				size_t count = potential->_subviews.Count();
				for(size_t i=0; i<count; i++)
				{
					View *view = potential->_subviews.ObjectAtIndex<View>(i);
					Vector2 transformed = std::move(view->ConvertPointFromView(point, nullptr));
					
					if(view->_interactionEnabled && view->PointInside(transformed, event))
					{
						potential = view;
						traverse = true;
						break;
					}
				}
			}
			
			return (potential && potential->_interactionEnabled) ? potential : nullptr;
		}
		
		bool View::PointInside(const Vector2& point, Event *event)
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
			NeedsLayoutUpdate();
		}
		
		void View::ViewHierarchyChanged()
		{
			size_t count = _subviews.Count();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				subview->_widget = _widget;
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
			NeedsLayoutUpdate();
		}
		
		void View::RemoveSubview(View *subview)
		{
			size_t index = _subviews.IndexOfObject(subview);
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
				
				NeedsLayoutUpdate();
			}
		}
		
		void View::RemoveAllSubviews()
		{
			size_t count = _subviews.Count();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				
				WillRemoveSubview(subview);
				subview->WillMoveToSuperview(nullptr);
				
				subview->_superview = 0;
				subview->_widget = 0;
				
				subview->DidMoveToSuperview(nullptr);
			}
			
			_subviews.RemoveAllObjects();
			NeedsLayoutUpdate();
		}
		
		void View::RemoveFromSuperview()
		{
			_superview->RemoveSubview(this);
		}
		
		
		void View::DidAddSubview(View *subview)
		{}
		void View::WillRemoveSubview(View *subview)
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
			Vector2 size = std::move(SizeThatFits());
			_frame.width  = size.x;
			_frame.height = size.y;
			
			SetFrame(_frame);
			NeedsLayoutUpdate();
		}
		
		Vector2 View::SizeThatFits()
		{
			return _frame.Size();
		}
		
		void View::SetFrame(const Rect& frame)
		{
			_frame = frame;
			
			_bounds.width  = frame.width;
			_bounds.height = frame.height;
			
			NeedsLayoutUpdate();
		}
		
		void View::SetBounds(const Rect& bounds)
		{
			_frame.width  = bounds.width;
			_frame.height = bounds.height;
			
			_bounds = bounds;
			
			NeedsLayoutUpdate();
		}
		
		void View::NeedsLayoutUpdate()
		{
			_dirtyLayout = true;
			
			size_t count = _subviews.Count();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				subview->NeedsLayoutUpdate();
			}
		}
		
		// ---------------------
		// MARK: -
		// MARK: Rendering
		// ---------------------
		
		void View::CalculateScissorRect()
		{
			float serverHeight = (_widget && _widget->_server) ? _widget->_server->Height() : 0.0f;
			Vector2 origin = _frame.Origin();
			
			_clippingView = nullptr;
			
			View *view = _superview;
			while(view)
			{
				if(!_clippingView && view->_clipSubviews)
					_clippingView = view;
				
				origin.x += view->_frame.x;
				origin.y += view->_frame.y;
				
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
			
			if(_clippingView)
			{
				_scissorRect.x     = std::max(_scissorRect.x, _clippingView->_scissorRect.x);
				_scissorRect.width = std::min(_scissorRect.width, _clippingView->_scissorRect.Right() - _scissorRect.x);
				
				_scissorRect.y      = std::max(_scissorRect.y, _clippingView->_scissorRect.y);
				_scissorRect.height = std::min(_scissorRect.height, _clippingView->_scissorRect.Top() - _scissorRect.y);
			}
		}
		
		void View::Update()
		{
			if(_dirtyLayout)
			{
				Vector2 converted = ConvertPointToView(Vector2(), nullptr);
				float serverHeight = 0.0f;
				
				if(_superview)
				{
					_intermediateTransform = _superview->_intermediateTransform * transform;
				}
				else if(_widget)
				{
					_intermediateTransform = _widget->transform * transform;
				}
				
				if(_widget)
				{
					converted.x += _widget->_frame.x;
					converted.y += _widget->_frame.y;
					
					if(_widget->_server)
						serverHeight = _widget->_server->Height();
				}
				
				_finalTransform = _intermediateTransform;
				_finalTransform.Translate(Vector3(converted.x, serverHeight - _frame.height - converted.y, 0.0f));
				
				_dirtyLayout = false;
				
				CalculateScissorRect();
			}
			
			if(_widget && _widget->_server && _widget->_server->DrawDebugFrames())
			{
				Rect frame = ConvertRectToView(Bounds(), nullptr);
				
				if(_widget)
				{
					frame.x += _widget->_frame.x;
					frame.y += _widget->_frame.y;
				}
				
				Debug::AddLinePoint(Vector2(frame.Left(), frame.Top()), Color::Red());
				Debug::AddLinePoint(Vector2(frame.Right(), frame.Top()), Color::Red());
				Debug::AddLinePoint(Vector2(frame.Right(), frame.Bottom()), Color::Red());
				Debug::AddLinePoint(Vector2(frame.Left(), frame.Bottom()), Color::Red());
				Debug::AddLinePoint(Vector2(frame.Left(), frame.Top()), Color::Red());
				Debug::EndLine();
			}
		}
		
		void View::PopulateRenderingObject(RenderingObject& object)
		{
			object.material = _material;
			object.transform = &_finalTransform;
			object.scissorTest = true;
			
			if(_clippingView)
			{
				object.scissorRect = _clippingView->_scissorRect;
			}
			else if(_widget)
			{
				float serverHeight = (_widget->_server) ? _widget->_server->Height() : 0.0f;
				
				object.scissorRect = _widget->Frame();
				object.scissorRect.y = serverHeight - object.scissorRect.height - object.scissorRect.y;
			}
		}
		
		void View::PrepareRendering(RenderingObject& object)
		{
			Update();
			PopulateRenderingObject(object);
		}
		
		bool View::Render(RenderingObject& object)
		{
			return false;
		}
		
		void View::Render(Renderer *renderer)
		{
			RenderingObject object;
			PrepareRendering(object);
			
			if(Render(object))
				renderer->RenderObject(std::move(object));
			
			RenderChilds(renderer);
		}
		
		void View::RenderChilds(Renderer *renderer)
		{
			size_t count = _subviews.Count();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				subview->Render(renderer);
			}
		}
	}
}
