//
//  RNUIScrollView.h
//  Rayne
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISCROLLVIEW_H__
#define __RAYNE_UISCROLLVIEW_H__

#include "RNBase.h"
#include "RNUIView.h"
#include "RNUIScroller.h"

namespace RN
{
	namespace UI
	{
		class ScrollView;
		class ScrollerKnob;
		
		class ScrollViewDelegate
		{
		public:
			RNAPI virtual void ScrollViewDidScroll(ScrollView *scrollView) {}
		};
		
		class ScrollView : public View
		{
		public:
			friend class ScrollerKnob;
			
			RNAPI ScrollView();
			RNAPI ~ScrollView();
			
			RNAPI void SetDelegate(ScrollViewDelegate *delegate);
			RNAPI void CalculateContentSize();
			
			RNAPI void SetContentOffset(const Vector2& offset);
			RNAPI void SetContentSize(const Vector2& size);
			RNAPI void SetVerticalScroller(Scroller *scroller);
			
			RNAPI void ScrollWheel(Event *event) override;
			RNAPI void SetFrame(const Rect& frame) override;
			
			RNAPI void DidAddSubview(View *subview) override;
			RNAPI void DidBringSubviewToFront(View *subview) override;
			
			RNAPI const Vector2& GetContentOffset() const { return _offset; }
			RNAPI const Vector2& GetContentSize() const { return _size; }
			
		private:
			void AdjustScroller();
			void MoveScroller(Vector2 delta);
			
			ScrollViewDelegate *_delegate;
			
			Scroller *_verticalScroller;
			
			Vector2 _offset;
			Vector2 _size;
			Vector2 _end;
			
			RNDeclareMeta(ScrollView)
		};
	}
}

#endif /* __RAYNE_UISCROLLVIEW_H__ */
