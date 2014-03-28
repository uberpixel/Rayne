//
//  RNWidget.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIWidget.h"
#include "RNUIServer.h"
#include "RNUIView.h"
#include "RNUIStyle.h"
#include "RNUIWidgetInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Widget, Responder)

		Widget::Widget(Style style)
		{
			Initialize(style);
		}
		
		Widget::Widget(Style style, const Rect& frame) :
			_frame(frame)
		{
			Initialize(style);
		}
		
		Widget::~Widget()
		{
			_contentView->Release();
			SafeRelease(_backgroundView);
		}
		
		void Widget::Initialize(Style style)
		{
			_style     = style;
			_hasShadow = (_style != Style::Borderless);
			_level     = kRNUIWidgetLevelNormal;
			
			_backgroundView = CreateBackgroundView();
			_contentView    = CreateContentView();
			
			_server         = nullptr;
			_firstResponder = nullptr;
			
			_minimumSize = Vector2(0.0f, 0.0f);
			_maximumSize = Vector2(FLT_MAX, FLT_MAX);
		}
		
		
		// ---------------------
		// MARK: -
		// MARK: Content handling
		// ---------------------
		
		View *Widget::CreateContentView()
		{
			View *view = new View(Rect(Vector2(0.0f), GetContentSize()));
			view->_widget = this;
			
			return view;
		}
		
		WidgetBackgroundView *Widget::CreateBackgroundView()
		{
			if(_style == Style::Borderless)
				return nullptr;
			
			UI::Style *styleSheet = UI::Style::GetSharedInstance();
			Dictionary *style = nullptr;
			
			if(_style & Style::Titled)
				style = styleSheet->GetWindowStyleWithKeyPath(RNCSTR("window.titled"));
			else
				style = styleSheet->GetWindowStyleWithKeyPath(RNCSTR("window.untitled"));
			
			WidgetBackgroundView *background = new WidgetBackgroundView(this, _style, style);
			background->_widget = this;
			background->_clipWithWidget = false;
			
			background->ViewHierarchyChanged();
			background->SetFrame(Rect(0.0f, 0.0f, _frame.width, _frame.height));
			
			return background;
		}
		
		Vector2 Widget::GetContentSize() const
		{
			return Vector2(_frame.width, _frame.height);
		}
		
		void Widget::SetContentView(View *view)
		{
			_contentView->Release();
			
			_contentView = view ? view->Retain() : CreateContentView();
			_contentView->_widget = this;
			_contentView->ViewHierarchyChanged();
			
			ConstraintContentView();
		}
		
		void Widget::SetMinimumSize(const Vector2& size)
		{
			_minimumSize = size;
			
			ConstraintFrame();
			ConstraintContentView();
		}
		
		void Widget::SetMaximumSize(const Vector2& size)
		{
			_maximumSize = size;
			
			ConstraintFrame();
			ConstraintContentView();
		}
		
		void Widget::SetFrame(const Rect& frame)
		{
			if(_frame != frame)
			{
				_frame = frame;
				
				if(_backgroundView)
					_backgroundView->SetFrame(Rect(0.0f, 0.0f, frame.width, frame.height));
			
				ConstraintFrame();
				ConstraintContentView();
			}
		}
		
		void Widget::SetTitle(String *title)
		{
			if(_backgroundView)
				_backgroundView->SetTitle(title);
		}
		
		void Widget::SetTransform(const Matrix& transform)
		{
			_transform = transform;
		}
		
		void Widget::SetContentSize(const Vector2& size)
		{
			Rect frame;
			frame.width  = size.x;
			frame.height = size.y;
			SetFrame(frame);
		}
		
		void Widget::ConstraintContentView()
		{
			Rect frame = _contentView->GetFrame();
			Vector2 size = GetContentSize();
			
			frame.width  = size.x;
			frame.height = size.y;
			
			_contentView->SetFrame(frame);
		}
		
		void Widget::ConstraintFrame()
		{
			Rect frame = _frame;
			frame.width  = std::min(_maximumSize.x, std::max(_minimumSize.x, frame.width));
			frame.height = std::min(_maximumSize.y, std::max(_minimumSize.y, frame.height));
			
			_frame = frame;
		}
		
		// ---------------------
		// MARK: -
		// MARK: First responder
		// ---------------------
		
		void Widget::MakeKeyWidget()
		{
			if(!_server)
				return;
			
			_server->SetKeyWidget(this);
		}
		
		bool Widget::MakeFirstResponder(Responder *responder)
		{
			if(responder == _firstResponder)
				return true;
			
			if(_firstResponder && _firstResponder != this)
			{
				bool result = _firstResponder->CanResignFirstReponder();
				if(!result)
					return false;
				
				_firstResponder->ResignFirstResponder();
				_firstResponder = nullptr;
			}
			
			if(responder)
			{
				bool result = responder->CanBecomeFirstResponder();
				if(!result)
					return false;
				
				_firstResponder = responder;
				_firstResponder->BecomeFirstResponder();
			}
			
			return true;
		}
		
		void Widget::ForceResignFirstResponder()
		{
			_firstResponder = nullptr;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Layering
		// ---------------------
		
		void Widget::Open()
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
				Open();
				return;
			}
			
			_server->MoveWidgetToFront(this);
		}
		
		View *Widget::PerformHitTest(const Vector2& position, Event *event)
		{
			if(_frame.ContainsPoint(position))
			{
				Vector2 transformed = position;
				transformed.x -= _frame.x;
				transformed.y -= _frame.y;
				
				return _contentView->HitTest(transformed, event);
			}
			
			// Check the background view
			if(_backgroundView)
			{
				const EdgeInsets& border = _backgroundView->GetBorder();
				Rect extendedFrame = _frame;
				
				extendedFrame.x     -= border.left;
				extendedFrame.width += border.left + border.right;
				
				extendedFrame.y     -= border.top;
				extendedFrame.width += border.bottom;
				
				if(extendedFrame.ContainsPoint(position))
				{
					Vector2 transformed = position;
					transformed.x -= _frame.x;
					transformed.y -= _frame.y;
					
					return _backgroundView->HitTest(transformed, event);
				}
			}
			
			return nullptr;
		}
		
		void Widget::SetWidgetLevel(int32 level)
		{
			_level = level;
			
			if(_server)
				_server->SortWidgets();
		}
		
		// ---------------------
		// MARK: -
		// MARK: Layout engine
		// ---------------------
		
		void Widget::SetNeedsLayoutUpdate()
		{
			_contentView->SetNeedsLayoutUpdate();
		}
		
		void Widget::Center()
		{
			Rect frame = GetFrame();
			Vector2 extents = Vector2(Server::GetSharedInstance()->GetWidth(), Server::GetSharedInstance()->GetHeight());
			
			frame.x = extents.x * 0.5f - frame.width * 0.5f;
			frame.y = extents.y * 0.5f - frame.height * 0.5f;
			
			SetFrame(frame);
		}
		
		void Widget::Update()
		{
			if(_backgroundView)
				_backgroundView->UpdateRecursively();
			
			_contentView->UpdateRecursively();
		}
		
		void Widget::Render(Renderer *renderer)
		{
			if(_backgroundView)
				_backgroundView->DrawRecursively(renderer);
			
			_contentView->DrawRecursively(renderer);
		}
	}
}
