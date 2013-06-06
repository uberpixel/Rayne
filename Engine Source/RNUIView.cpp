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
			_frame(frame)
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
			
			_superview = 0;
			_widget = 0;
			_dirtyLayout = true;
			
			_material = new Material(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNViewShaderResourceName));
			_material->depthtest = false;
			_material->depthwrite = false;
			_material->blending = true;
			_material->lighting = false;
			_material->diffuse = Color(1.0f, 1.0f, 1.0f, 1.0f);
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
		
		Vector2 View::ConvertPointToView(View *view, const Vector2& point)
		{
			Vector2 converted = point;
			View *temp = _superview;
			
			while(temp)
			{
				converted.x += temp->_frame.x;
				converted.y += temp->_frame.y;
				
				if(temp == view)
					break;
				
				temp = temp->_superview;
			}
			
			if(!view && _widget)
			{
				converted.x += _widget->_frame.x;
				converted.y += _widget->_frame.y;
			}
			
			return converted;
		}
		
		Vector2 View::ConvertPointFromView(View *view, const Vector2& point)
		{
			std::vector<View *> path;
			View *temp = _superview;
			
			while(temp)
			{
				path.push_back(temp);
				
				if(temp == view)
					break;
				
				temp = temp->_superview;
			}
			

			Vector2 converted = point;
			
			if(!view && _widget)
			{
				converted.x -= _widget->_frame.x;
				converted.y -= _widget->_frame.y;
			}
			
			for(View *temp : path)
			{
				converted.x -= temp->_frame.x;
				converted.y -= temp->_frame.y;
			}
			
			return converted;
		}
		
		
		Rect View::ConvertRectToView(View *view, const Rect& frame)
		{
			Rect converted = frame;
			Vector2 point = ConvertPointToView(view, Vector2(frame.x, frame.y));
			
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
			if(_superview)
				_widget = _superview->_widget;
			
			machine_uint count = _subviews.Count();
			for(machine_uint i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				subview->ViewHierarchyChanged();
			}
			
			_dirtyLayout = true;
		}
		
		void View::AddSubview(View *subview)
		{
			if(subview->_superview)
				subview->RemoveFromSuperview();
			
			_subviews.AddObject(subview->Retain());
			
			subview->_superview = this;
			subview->_widget = _widget;
			
			NeedsLayoutUpdate();
		}
		
		void View::RemoveSubview(View *subview)
		{
			machine_uint index = _subviews.IndexOfObject(subview);
			if(index != kRNNotFound)
			{
				_subviews.RemoveObjectAtIndex(index);
				
				subview->_superview = 0;
				subview->_widget = 0;
				
				NeedsLayoutUpdate();
			}
		}
		
		void View::RemoveAllSubviews()
		{
			machine_uint count = _subviews.Count();
			for(machine_uint i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				
				subview->_superview = 0;
				subview->_widget = 0;
			}
			
			_subviews.RemoveAllObjects();
			NeedsLayoutUpdate();
		}
		
		void View::RemoveFromSuperview()
		{
			_superview->RemoveSubview(this);
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Layout engine
		// ---------------------
		
		void View::SetFrame(const Rect& frame)
		{
			_frame = frame;
			NeedsLayoutUpdate();
		}
		
		void View::NeedsLayoutUpdate()
		{
			_dirtyLayout = true;
			
			machine_uint count = _subviews.Count();
			for(machine_uint i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				subview->NeedsLayoutUpdate();
			}
		}
		
		// ---------------------
		// MARK: -
		// MARK: Rendering
		// ---------------------
		
		void View::Update()
		{
			if(_dirtyLayout)
			{
				Rect converted = ConvertRectToView(0, _frame);
				float serverHeight = (_widget && _widget->_server) ? _widget->_server->Height() : 0.0f;
				
				if(_superview)
				{
					_intermediateTransform = _superview->_intermediateTransform * transform;
				}
				else if(_widget)
				{
					_intermediateTransform = _widget->transform * transform;
				}
				
				_finalTransform = _intermediateTransform;
				_finalTransform.Translate(Vector3(converted.x, serverHeight - _frame.height - converted.y, 0.0f));
				
				_dirtyLayout = false;
			}
		}
		
		void View::PrepareRendering(RenderingObject& object)
		{
			Update();
			
			object.material = _material;
			object.transform = &_finalTransform;
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
				renderer->RenderObject(object);
			
			machine_uint count = _subviews.Count();
			for(machine_uint i=0; i<count; i++)
			{
				View *subview = _subviews.ObjectAtIndex<View>(i);
				subview->Render(renderer);
			}
		}
	}
}
