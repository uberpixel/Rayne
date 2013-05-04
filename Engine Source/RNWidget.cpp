//
//  RNWidget.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWidget.h"
#include "RNUIServer.h"

namespace RN
{
	RNDeclareMeta(Widget)
	
	static Widget *__MainWidget = 0;
	
	Widget::Widget()
	{
		Initialize();
	}
	
	Widget::~Widget()
	{
		_contentView->Release();
	}
	
	void Widget::Initialize()
	{
		_contentView = EmptyContentView();
		_dirtyLayout = true;
		_server = 0;
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Content handling
	// ---------------------
	
	View *Widget::EmptyContentView()
	{
		View *view = new View(Rect(Vector2(0.0f), ContentSize()));
		return view->Autorelease();
	}
	
	Vector2 Widget::ContentSize() const
	{
		return Vector2(_frame.width, _frame.height);
	}
	
	void Widget::SetContentView(View *view)
	{
		_contentView->Release();
		
		_contentView = view ? view->Retain() : EmptyContentView();
		_contentView->_widget = this;
		
		_contentView->ViewHierarchyChanged();
		NeedsLayoutUpdate();
	}
	
	void Widget::SetMinimumSize(const Vector2& size)
	{
		_minimumSize = size;
		NeedsLayoutUpdate();
	}
	
	void Widget::SetMaximumSize(const Vector2& size)
	{
		_maximumSize = size;
		NeedsLayoutUpdate();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Layering
	// ---------------------
	
	void Widget::Show()
	{
		if(!_server)
		{
			_server = UIServer::SharedInstance();
			_server->AddWidget(this);
		}
	}
	
	void Widget::Close()
	{
		if(_server)
		{
			_server->RemoveWidget(this);
			_server = 0;
		}
	}
	
	void Widget::OrderFront()
	{
		if(_server)
		{
			_server->MoveWidgetToFront(this);
		}
	}
	
	// ---------------------
	// MARK: -
	// MARK: Layout engine
	// ---------------------
	
	void Widget::NeedsLayoutUpdate()
	{
		_dirtyLayout = true;
	}
	
	void Widget::UpdateLayout()
	{
		
		_dirtyLayout = false;
	}
	
	void Widget::Render(Renderer *renderer)
	{
		if(_dirtyLayout)
			UpdateLayout();
		
		_contentView->Render(renderer);
	}
}
