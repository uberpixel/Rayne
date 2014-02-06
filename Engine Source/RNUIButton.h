//
//  RNUIButton.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIBUTTON_H__
#define __RAYNE_UIBUTTON_H__

#include "RNBase.h"
#include "RNMesh.h"
#include "RNString.h"
#include "RNUIImage.h"
#include "RNUIControl.h"
#include "RNUIImageView.h"
#include "RNUILabel.h"

namespace RN
{
	namespace UI
	{
		class Button : public Control
		{
		public:
			enum class Type
			{
				Bezel,
				CheckBox,
				DisclosureTriangle
			};
			
			enum class Behavior
			{
				Momentarily,
				Switch
			};
			
			RNAPI Button();
			RNAPI Button(Type type);
			RNAPI Button(Dictionary *style);
			RNAPI ~Button() override;
			
			RNAPI static Button *WithType(Type type);
			
			RNAPI void SetTitleForState(String *title, State state);
			RNAPI void SetBackgroundImageForState(Image *image, State state);
			RNAPI void SetImageForState(Image *image, State state);
			RNAPI void SetBehavior(Behavior behavior);
			RNAPI void SetImagePosition(ImagePosition position);
			RNAPI void SetContentInsets(const EdgeInsets& insets);
			
			RNAPI void SetFrame(const Rect& frame) override;
			RNAPI Vector2 GetSizeThatFits() override;
			
		protected:
			RNAPI bool PostEvent(EventType event) override;
			RNAPI void StateChanged(State state) override;
			
			RNAPI void LayoutSubviews() override;
			
		private:
			void Initialize();
			void InitializeFromStyle(Dictionary *style);

			ControlStateStore<Image> _backgroundImages;
			ControlStateStore<Image> _images;
			ControlStateStore<String> _titles;
			
			ImagePosition _position;
			Behavior _behavior;
			
			ImageView *_backgroundImage;
			ImageView *_image;
			Label *_label;
			
			EdgeInsets _contentInsets;
			
			String *_currentTitle;
			Image *_currentImage;
			
			RNDeclareMeta(Button, Control)
		};
	}
}

#endif /* __RAYNE_UIBUTTON_H__ */
