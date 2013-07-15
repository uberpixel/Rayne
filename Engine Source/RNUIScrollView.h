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
		class ScrollView : public View
		{
		public:
			ScrollView();
			~ScrollView();
			
			void CalculateContentSize();
			
			void SetContentOffset(const Vector2& offset);
			void SetContentSize(const Vector2& size);
			
			void ScrollWheel(Event *event) override;
			void SetFrame(const Rect& frame) override;
			
		private:
			Vector2 _offset;
			Vector2 _size;
			Vector2 _end;
			
			RNDefineMeta(ScrollView, View)
		};
	}
}

#endif /* __RAYNE_UISCROLLVIEW_H__ */
