//
//  RNWidget.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIWidget.h"
#include "RNUIServer.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Widget)

		Widget::Widget()
		{
			Initialize();
		}
		
		Widget::Widget(const Rect& frame) :
			_frame(frame)
		{
			Initialize();
		}
		
		Widget::~Widget()
		{
			_contentView->Release();
		}
		
		void Widget::Initialize()
		{
			_contentView = EmptyContentView()->Retain();
			_dirtyLayout = true;
			_server = 0;
			
			_minimumSize = Vector2(0.0f, 0.0f);
			_maximumSize = Vector2(FLT_MAX, FLT_MAX);
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Content handling
		// ---------------------
		
		View *Widget::EmptyContentView()
		{
			View *view = new View(Rect(Vector2(0.0f), ContentSize()));
			view->_widget = this;
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
			
			ConstraintContentView();
			NeedsLayoutUpdate();
		}
		
		void Widget::SetMinimumSize(const Vector2& size)
		{
			_minimumSize = size;
			ConstraintFrame();
		}
		
		void Widget::SetMaximumSize(const Vector2& size)
		{
			_maximumSize = size;
			ConstraintFrame();
		}
		
		void Widget::SetFrame(const Rect& frame)
		{
			_frame = frame;
			ConstraintFrame();
		}
		
		void Widget::ConstraintContentView()
		{
			return;
			
			Rect frame = _contentView->Frame();
			Vector2 size = ContentSize();
			
			frame.width  = size.x;
			frame.height = size.y;
			
			_contentView->SetFrame(frame);
		}
		
		void Widget::ConstraintFrame()
		{
			Rect frame = _frame;
			frame.width  = MIN(_maximumSize.x, MAX(_minimumSize.x, frame.width));
			frame.height = MIN(_maximumSize.y, MAX(_minimumSize.y, frame.height));
			
			if(frame != _frame)
			{
				_frame = frame;
				
				ConstraintContentView();
				NeedsLayoutUpdate();
			}
		}
		
		// ---------------------
		// MARK: -
		// MARK: Layering
		// ---------------------
		
		void Widget::Show()
		{
			if(_server)
			{
				OrderFront();
				return;
			}
			
			Server::GetSharedInstance()->AddWidget(this);
		}
		
		void Widget::Close()
		{
			if(_server)
				_server->RemoveWidget(this);
		}
		
		void Widget::OrderFront()
		{
			if(!_server)
			{
				Show();
				return;
			}
			
			_server->MoveWidgetToFront(this);
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
			if(_server)
			{
				_finalTransform = transform;
				_finalTransform.Translate(Vector3(_frame.x, _server->Height() - _frame.height - _frame.y, 0.0f));
				_finalTransform.Scale(Vector3(_frame.width, _frame.height, 1.0f));
				
				_dirtyLayout = false;
			}
		}
		
		void Widget::Render(Renderer *renderer)
		{
			if(!_server)
				return;
			
			if(_dirtyLayout)
				UpdateLayout();
			
			_contentView->UpdateAndDrawChilds(renderer);
		}
	}
}
