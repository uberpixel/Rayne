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
				PushButton,
				CheckBox
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
			
			void Update() override;
			
		private:
			void Initialize();
			
			bool ActivateBackgroundImage(State state);
			bool ActivateImage(State state);
			bool ActivateTitle(State state);
			
			std::map<State, Image *> _backgroundImages;
			std::map<State, Image *> _images;
			std::map<State, String *> _titles;
			
			bool _dirty;
			
			ImagePosition _position;
			Behavior _behavior;
			
			ImageView *_backgroundImage;
			ImageView *_image;
			Label *_label;
			
			EdgeInsets _contentInsets;
			
			Image *_currentBackground;
			Image *_currentImage;
			String *_currentTitle;
			
			RNDefineMeta(Button, Control)
		};
	}
}

#endif /* __RAYNE_UIBUTTON_H__ */
