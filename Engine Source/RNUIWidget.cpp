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
#include "RNApplication.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Widget, Responder)
		
		struct WidgetInternals
		{
			WidgetInternals() :
				firstResponder(nullptr),
				minimumSize(Vector2(0.0f)),
				maximumSize(Vector2(FLT_MAX))
			{}
			
			~WidgetInternals()
			{}
			
			Widget::Style style;
			bool hasShadow;
			bool canBecomeKeyWidget;
			
			Responder *firstResponder;
			
			Vector2 minimumSize;
			Vector2 maximumSize;
		};

		Widget::Widget(Style style) :
			Widget(style, Rect())
		{}
		
		Widget::Widget(Style style, const Rect &frame) :
			_frame(frame),
			_server(nullptr),
			_level(kRNUIWidgetLevelNormal),
			_delegate(nullptr)
		{
			_internals->style     = style;
			_internals->hasShadow = (style != Style::Borderless);
			
			_backgroundView = CreateBackgroundView();
			_contentView    = CreateContentView();
		}
		
		Widget::~Widget()
		{
			_contentView->Release();
			SafeRelease(_backgroundView);
		}
		
		// ---------------------
		// MARK: -
		// MARK: Content handling
		// ---------------------
		
		View *Widget::CreateContentView()
		{
			View *view = new View(Rect(Vector2(), GetContentSize()));
			view->_widget = this;
			
			return view;
		}
		
		WidgetBackgroundView *Widget::CreateBackgroundView()
		{
			if(_internals->style == Style::Borderless)
				return nullptr;
			
			UI::Style *styleSheet = UI::Style::GetSharedInstance();
			Dictionary *style = nullptr;
			
			if(_internals->style & Style::Titled)
				style = styleSheet->GetWindowStyleWithKeyPath(RNCSTR("window.titled"));
			else
				style = styleSheet->GetWindowStyleWithKeyPath(RNCSTR("window.untitled"));
			
			WidgetBackgroundView *background = new WidgetBackgroundView(this, _internals->style, style);
			background->_widget         = this;
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
		
		void Widget::SetMinimumSize(const Vector2 &size)
		{
			_internals->minimumSize = size;
			
			ConstraintFrame();
			ConstraintContentView();
		}
		
		void Widget::SetMaximumSize(const Vector2 &size)
		{
			_internals->maximumSize = size;
			
			ConstraintFrame();
			ConstraintContentView();
		}
		
		void Widget::SetFrame(const Rect &frame)
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
		
		void Widget::SetTransform(const Matrix &transform)
		{
			_transform = transform;
			SetNeedsLayoutUpdate();
		}
		
		void Widget::SetContentSize(const Vector2 &size)
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
			SetNeedsLayoutUpdate();
		}
		
		void Widget::ConstraintFrame()
		{
			Rect frame = _frame;
			frame.width  = std::min(_internals->maximumSize.x, std::max(_internals->minimumSize.x, frame.width));
			frame.height = std::min(_internals->maximumSize.y, std::max(_internals->minimumSize.y, frame.height));
			
			_frame = frame;
		}
		
		void Widget::SetCanBecomeKeyWidget(bool canBecome)
		{
			_internals->canBecomeKeyWidget = canBecome;
			
			if(!canBecome && _server && _server->_keyWidget == this)
				_server->SetKeyWidget(nullptr);
		}
		
		void Widget::SetDelegate(Delegate *delegate)
		{
			_delegate = delegate;
		}
		
		// ---------------------
		// MARK: -
		// MARK: First responder
		// ---------------------
		
		bool Widget::MakeKeyWidget()
		{
			if(!_server || !_internals->canBecomeKeyWidget)
				return false;
			
			_server->SetKeyWidget(this);
			return true;
		}
		
		bool Widget::MakeFirstResponder(Responder *responder)
		{
			if(responder == _internals->firstResponder)
				return true;
			
			if(_internals->firstResponder && _internals->firstResponder != this)
			{
				bool result = _internals->firstResponder->CanResignFirstReponder();
				if(!result)
					return false;
				
				_internals->firstResponder->ResignFirstResponder();
				_internals->firstResponder = nullptr;
			}
			
			if(responder)
			{
				bool result = responder->CanBecomeFirstResponder();
				if(!result)
					return false;
				
				_internals->firstResponder = responder;
				_internals->firstResponder->BecomeFirstResponder();
			}
			
			return true;
		}
		
		void Widget::ForceResignFirstResponder()
		{
			_internals->firstResponder = nullptr;
		}
		
		Responder *Widget::GetNextResponder() const
		{
			return Application::GetSharedInstance();
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
			
			if(_delegate)
				_delegate->WidgetDidOpen(this);
		}
		
		void Widget::Close()
		{
			if(_delegate)
			{
				if(!_delegate->WidgetCanClose(this))
					return;
				
				_delegate->WidgetWillClose(this);
			}
			
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
		
		View *Widget::PerformHitTest(const Vector2 &position, Event *event)
		{
			if(_frame.ContainsPoint(position))
			{
				Vector2 transformed = _contentView->ConvertPointFromBase(position);
				return _contentView->HitTest(transformed, event);
			}
			
			// Check the background view
			if(_backgroundView)
			{
				const EdgeInsets &border = _backgroundView->GetBorder();
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
		// MARK: Delegate related
		// ---------------------
		
		bool Widget::CanBecomeKeyWidget() const
		{
			if(!_internals->canBecomeKeyWidget)
				return false;
			
			if(_delegate)
				return _delegate->WidgetCanBecomeKey(const_cast<Widget *>(this));
			
			return true;
		}
		
		bool Widget::CanResignKeyWidget() const
		{
			if(_delegate)
				return _delegate->WidgetCanResignKey(const_cast<Widget *>(this));
			
			return true;
		}
		
		void Widget::AcceptKey()
		{
			if(_delegate)
				_delegate->WidgetDidBecomeKey(this);
		}
		
		void Widget::ResignKey()
		{
			if(_delegate)
				_delegate->WidgetDidResignKey(this);
		}
		
		// ---------------------
		// MARK: -
		// MARK: Getter
		// ---------------------
		
		View *Widget::GetContentView() const
		{
			return _contentView;
		}
		
		Responder *Widget::GetFirstResponder() const
		{
			return _internals->firstResponder;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Layout engine
		// ---------------------
		
		void Widget::SetNeedsLayoutUpdate()
		{
			_contentView->SetNeedsLayoutUpdate();
			
			if(_delegate)
				_delegate->WidgetLayoutContent(this);
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
