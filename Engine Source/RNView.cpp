//
//  RNView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNView.h"
#include "RNWidget.h"

namespace RN
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
		RemoveAllSubviews();
		
		_material->Release();
	}
	
	
	void View::Initialize()
	{
		_superview = 0;
		_widget = 0;
		_dirtyLayout = false;
		
		_material = new Material();
		_material->depthtest = false;
		_material->depthwrite = false;
		_material->blending = true;
	}
	
	
	
	void View::ViewHierarchyChanged()
	{
		if(_superview)
		{
			_widget = _superview->_widget;
		}
		
		machine_uint count = _subviews.Count();
		for(machine_uint i=0; i<count; i++)
		{
			View *subview = _subviews.ObjectAtIndex(i);
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
			subview->Release();
			
			NeedsLayoutUpdate();
		}
	}
	
	void View::RemoveAllSubviews()
	{
		machine_uint count = _subviews.Count();
		for(machine_uint i=0; i<count; i++)
		{
			View *subview = _subviews.ObjectAtIndex(i);
			
			subview->_superview = 0;
			subview->_widget = 0;
			subview->Release();
		}
		
		_subviews.RemoveAllObjects();
		NeedsLayoutUpdate();
	}
	
	void View::RemoveFromSuperview()
	{
		_superview->RemoveSubview(this);
	}
	
	
	
	
	void View::NeedsLayoutUpdate()
	{
		_dirtyLayout = true;
		
		machine_uint count = _subviews.Count();
		for(machine_uint i=0; i<count; i++)
		{
			View *subview = _subviews.ObjectAtIndex(i);
			subview->NeedsLayoutUpdate();
		}
	}
	
	void View::Update()
	{
		if(_dirtyLayout)
		{
			_finalTransform = _transform;
			
			_dirtyLayout = false;
		}
	}
	
	bool View::Render(RenderingObject& object)
	{
		Update();
		
		object.material = _material;
		object.transform = &_finalTransform;
		return false;
	}
	
	void View::Render(Renderer *renderer)
	{
		RenderingObject object;
		if(Render(object))
		{
			renderer->RenderObject(object);
		}
		
		machine_uint count = _subviews.Count();
		for(machine_uint i=0; i<count; i++)
		{
			View *subview = _subviews.ObjectAtIndex(i);
			subview->Render(renderer);
		}
	}
}
