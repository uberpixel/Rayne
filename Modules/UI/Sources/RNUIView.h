//
//  RNUIView.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UILAYER_H_
#define __RAYNE_UILAYER_H_

#include "RNUIConfig.h"
#include "RNUIContext.h"
#include "RNUIEdgeInsets.h"

namespace RN
{
	namespace UI
	{
		class Window;
		class View : public Object
		{
		public:
			friend class Window;

			UIAPI View();
			UIAPI View(const Rect &frame);
			UIAPI ~View();

			UIAPI Vector2 ConvertPointToView(const Vector2 &point, View *view) const;
			UIAPI Vector2 ConvertPointFromView(const Vector2 &point, View *view) const;

			UIAPI Vector2 ConvertPointToBase(const Vector2 &point) const;
			UIAPI Vector2 ConvertPointFromBase(const Vector2 &point) const;

			UIAPI Rect ConvertRectToView(const Rect &frame, View *view) const;
			UIAPI Rect ConvertRectFromView(const Rect &frame, View *view) const;

			UIAPI const Rect &GetFrame() const { return _frame; }
			UIAPI const Rect &GetBounds() const { return _bounds; }

			UIAPI void AddSubview(View *subview);
			UIAPI void RemoveSubview(View *subview);
			UIAPI void RemoveAllSubviews();
			UIAPI void RemoveFromSuperview();
			UIAPI void BringSubviewToFront(View *subview);
			UIAPI void SendSubviewToBack(View *subview);

			UIAPI virtual void SetFrame(const Rect &frame);
			UIAPI virtual void SetBounds(const Rect &bounds);

			UIAPI void SetBackgroundColor(const Color &color);

			UIAPI void SetNeedsLayout();
			UIAPI void LayoutIfNeeded();

			UIAPI virtual void DrawInContext(Context *context) const;

		protected:
			UIAPI virtual void LayoutSubviews();

			UIAPI virtual void DidAddSubview(View *subview);
			UIAPI virtual void WillRemoveSubview(View *subview);
			UIAPI virtual void DidBringSubviewToFront(View *subview);
			UIAPI virtual void DidSendSubviewToBack(View *subview);
			UIAPI virtual void WillMoveToSuperview(View *superview);
			UIAPI virtual void DidMoveToSuperview(View *superview);

		private:
			void __DrawInContext(Context *context) const;

			void ConvertPointToWindow(Vector2 &point) const;
			void ConvertPointFromWindow(Vector2 &point) const;

			void CalculateScissorRect();
			void ViewHierarchyChanged();

			Rect _bounds;
			Rect _frame;

			bool _clipsToBounds;
			bool _clipsToWindow;

			EdgeInsets _clipInsets;
			Rect _scissorRect;

			bool _dirtyLayout;

			Color _backgroundColor;

			Window *_window;

			View *_superview;
			View *_clippingView;
			Array *_subviews;

			Matrix _transform;
			Matrix _intermediateTransform;
			Matrix _finalTransform;

			RNDeclareMetaAPI(View, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILAYER_H_ */
