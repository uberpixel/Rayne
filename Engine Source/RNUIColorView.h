//
//  RNUIColorView.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICOLORVIEW_H__
#define __RAYNE_UICOLORVIEW_H__

#include "RNBase.h"
#include "RNUIView.h"
#include "RNUIControl.h"
#include "RNUIWidget.h"
#include "RNUIImageView.h"
#include "RNUIColorPicker.h"

namespace RN
{
	namespace UI
	{
		class ColorView : public Control
		{
		public:
			RNAPI ColorView();
			RNAPI ColorView(Dictionary *style);
			RNAPI ~ColorView();
			
			RNAPI void SetColor(const RN::Color &color);
			RNAPI const RN::Color &GetColor() const { return _color; }
			
			RNAPI void LayoutSubviews() override;
			
		protected:
			RNAPI void StateChanged(State state);
			RNAPI bool PostEvent(EventType event) override;
			
		private:
			void Initialize(Dictionary *style);
			void SetColorInternal(const RN::Color &color);
			
			RN::Color _color;
			ControlStateStore<Image> _borderImages;
			EdgeInsets _insets;
			
			ImageView *_border;
			View *_contentView;
			
			Widget *_colorPicker;
			
			RNDeclareMeta(ColorView)
		};
	}
}

#endif /* __RAYNE_UICOLORVIEW_H__ */
