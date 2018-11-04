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
		RNDefineMeta(View, Object)

		View::View() :
			_clipsToBounds(true),
			_clipsToWindow(true),
			_dirtyLayout(false),
			_subviews(new Array()),
			_window(nullptr),
			_superview(nullptr),
			_clippingView(nullptr),
			_backgroundColor(Color::ClearColor())
		{}
		View::View(const Rect &frame) :
			View()
		{
			SetFrame(frame);
		}
		View::~View()
		{
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->_superview = nullptr;
			}

			SafeRelease(_subviews);
		}


		// ---------------------
		// MARK: -
		// MARK: Coordinate systems
		// ---------------------

		void View::ConvertPointToWindow(Vector2 &point) const
		{
			View *view = _superview;
			while(view)
			{
				point.x += view->_frame.x + view->_bounds.x;
				point.y += view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}

			point.x += _frame.x + _bounds.x;
			point.y += _frame.y + _bounds.y;
		}

		void View::ConvertPointFromWindow(Vector2 &point) const
		{
			View *view = _superview;
			while(view)
			{
				point.x -= view->_frame.x + view->_bounds.x;
				point.y -= view->_frame.y + view->_bounds.y;

				view = view->_superview;
			}

			point.x -= _frame.x + _bounds.x;
			point.y -= _frame.y + _bounds.y;
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
			ConvertPointToWindow(converted);

			if(_window)
			{
				converted.x += _window->_frame.x;
				converted.y += _window->_frame.y;
			}

			return converted;
		}

		Vector2 View::ConvertPointFromBase(const Vector2 &point) const
		{
			Vector2 converted = point;
			if(_window)
			{
				converted.x -= _window->_frame.x;
				converted.y -= _window->_frame.y;
			}

			ConvertPointFromWindow(converted);
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

		void View::ViewHierarchyChanged()
		{
			size_t count = _subviews->GetCount();

			for(size_t i = 0; i < count; i++)
			{
				View *subview = _subviews->GetObjectAtIndex<View>(i);
				subview->_window = _window;
				subview->_clipsToWindow = _clipsToWindow;
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

			_subviews->AddObject(subview);

			subview->_superview = this;
			subview->_window = _window;

			subview->ViewHierarchyChanged();
			subview->DidMoveToSuperview(this);
			subview->Release();

			DidAddSubview(subview);
			SetNeedsLayout();
		}

		void View::RemoveSubview(View *subview)
		{
			size_t index = _subviews->GetIndexOfObject(subview);
			if(index != kRNNotFound)
			{
				WillRemoveSubview(subview);

				subview->Retain();
				subview->WillMoveToSuperview(nullptr);

				_subviews->RemoveObjectAtIndex(index);

				subview->_superview = nullptr;
				subview->_window = nullptr;

				subview->DidMoveToSuperview(nullptr);
				subview->Release();

				SetNeedsLayout();
			}
		}

		void View::RemoveAllSubviews()
		{
			size_t count = _subviews->GetCount();
			for(size_t i=0; i<count; i++)
			{
				View *subview = _subviews->GetObjectAtIndex<View>(i);

				WillRemoveSubview(subview);
				subview->WillMoveToSuperview(nullptr);

				subview->_superview = nullptr;
				subview->_window = nullptr;

				subview->DidMoveToSuperview(nullptr);
			}

			_subviews->RemoveAllObjects();
			SetNeedsLayout();
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

				_subviews->RemoveObject(subview);
				_subviews->AddObject(subview);

				subview->Release();
				DidBringSubviewToFront(subview);
			}
		}

		void View::SendSubviewToBack(View *subview)
		{
			if(subview->_superview == this)
			{
				subview->Retain();

				if(_subviews->GetCount() > 1)
				{
					_subviews->RemoveObject(subview);
					_subviews->InsertObjectAtIndex(subview, 0);
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
		// MARK: Properties
		// ---------------------

		void View::SetFrame(const Rect &frame)
		{
			Vector2 oldSize = _frame.GetSize();

			_frame = frame;

			_bounds.width  = frame.width;
			_bounds.height = frame.height;

			SetNeedsLayout();
			//ResizeSubviewsFromOldSize(oldSize);
		}

		void View::SetBounds(const Rect &bounds)
		{
/*			if(_frame.GetSize() != bounds.GetSize())
			{
				Vector2 size = _frame.GetSize();

				_frame.width  = bounds.width;
				_frame.height = bounds.height;

				//ResizeSubviewsFromOldSize(size);
			}*/

			_bounds = bounds;

			SetNeedsLayout();
		}

		void View::SetBackgroundColor(const Color &color)
		{
			_backgroundColor = color;
		}

		// ---------------------
		// MARK: -
		// MARK: Layout
		// ---------------------

		void View::SetNeedsLayout()
		{
			_dirtyLayout = true;
			_subviews->Enumerate<View>([&](View *subview, size_t index, bool &stop) {
				subview->SetNeedsLayout();
			});
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
				else if(_window)
				{
					_intermediateTransform = _window->_transform * _transform;
				}

				if(_window)
				{
					converted.x += _window->_frame.x;
					converted.y += _window->_frame.y;

					//if(_window->_server)
					//	serverHeight = _window->_server->GetHeight();
				}

				_finalTransform = _intermediateTransform;
				_finalTransform.Translate(Vector3(converted.x, serverHeight - _frame.height - converted.y, 0.0f));

				CalculateScissorRect();
				LayoutSubviews();

				_dirtyLayout = false;
			}

			// Update all children
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->LayoutIfNeeded();
			}
		}

		void View::CalculateScissorRect()
		{
			//Vector2 origin = _frame.GetOrigin();

			_clippingView = nullptr;

			/*View *view = _superview;
			while(view)
			{
				if(!_clippingView && view->_clipsToBounds)
					_clippingView = view;

				origin.x += view->_frame.x - view->_bounds.x;
				origin.y += view->_frame.y - view->_bounds.y;

				view = view->_superview;
			}*/

			_scissorRect.x = 0; //origin.x;
			_scissorRect.y = 0; //origin.y;
			_scissorRect.width  = _frame.width;
			_scissorRect.height = _frame.height;

			_scissorRect.x += _clipInsets.left;
			_scissorRect.width -= _clipInsets.left + _clipInsets.right;

			_scissorRect.y += _clipInsets.bottom;
			_scissorRect.height -= _clipInsets.bottom + _clipInsets.top;

			if(_clippingView)
			{
				_scissorRect.x = std::max(_scissorRect.x, _clippingView->_scissorRect.x);
				_scissorRect.width = std::min(_scissorRect.width, _clippingView->_scissorRect.GetRight() - _scissorRect.x);

				_scissorRect.y = std::max(_scissorRect.y, _clippingView->_scissorRect.y);
				_scissorRect.height = std::min(_scissorRect.height, _clippingView->_scissorRect.GetBottom() - _scissorRect.y);
			}
		}

		// ---------------------
		// MARK: -
		// MARK: Drawing
		// ---------------------

		void View::Draw(Context *context) const
		{}

		void View::DrawInContext(Context *context) const
		{
			if(_clipsToBounds)
				context->SetClipRect(_scissorRect);

			if(_backgroundColor.a > 0.05)
			{
				context->SetFillColor(_backgroundColor);
				context->FillRect(Rect(0, 0, _frame.width, _frame.height));
			}

			Draw(context);

			context->Translate(Vector2(_bounds.x, _bounds.y));

			// Draw all children
			size_t count = _subviews->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				View *child = _subviews->GetObjectAtIndex<View>(i);
				child->__DrawInContext(context);
			}
		}

		void View::__DrawInContext(Context *context) const
		{
			context->SaveState();
			context->Translate(Vector2(_frame.x, _frame.y));

			DrawInContext(context);

			context->RestoreState();
		}
	}
}
