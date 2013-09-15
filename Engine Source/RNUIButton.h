//
//  RNUIButton.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
				RoundedRect,
				Bezel,
				CheckBox,
				DisclosureTriangle
			};
			
			enum class Behavior
			{
				Momentarily,
				Switch
			};
			
			Button();
			Button(Dictionary *style);
			~Button() override;
			
			static Button *WithType(Type type);
			
			void SetTitleForState(String *title, State state);
			void SetBackgroundImageForState(Image *image, State state);
			void SetImageForState(Image *image, State state);
			void SetBehavior(Behavior behavior);
			void SetImagePosition(ImagePosition position);
			void SetContentInsets(const EdgeInsets& insets);
			
			void SetFrame(const Rect& frame) override;
			Vector2 SizeThatFits() override;
			
		protected:
			bool PostEvent(EventType event) override;
			void StateChanged(State state) override;
			
			void LayoutSubviews() override;
			
		private:
			void Initialize();

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
			
			RNDefineMeta(Button, Control)
		};
	}
}

#endif /* __RAYNE_UIBUTTON_H__ */
