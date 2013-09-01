//
//  RNUIScrollView.h
//  Rayne
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISCROLLVIEW_H__
#define __RAYNE_UISCROLLVIEW_H__

#include "RNBase.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class ScrollView;
		class ScrollViewDelegate
		{
		public:
			virtual void ScrollViewDidScroll(ScrollView *scrollView) {}
		};
		
		class ScrollView : public View
		{
		public:
			ScrollView();
			~ScrollView();
			
			void SetDelegate(ScrollViewDelegate *delegate);
			void CalculateContentSize();
			
			void SetContentOffset(const Vector2& offset);
			void SetContentSize(const Vector2& size);
			
			void ScrollWheel(Event *event) override;
			void SetFrame(const Rect& frame) override;
			
			const Vector2& GetContentOffset() const { return _offset; }
			const Vector2& GetContentSize() const { return _size; }
			
		private:
			ScrollViewDelegate *_delegate;
			
			Vector2 _offset;
			Vector2 _size;
			Vector2 _end;
			
			RNDefineMeta(ScrollView, View)
		};
	}
}

#endif /* __RAYNE_UISCROLLVIEW_H__ */
