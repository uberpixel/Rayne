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
				RoundedRect
			};
			
			Button();
			Button(Dictionary *style);
			~Button() override;
			
			static Button *WithType(Type type);
			
			void SetTitleForState(String *title, State state);
			void SetImageForState(Image *image, State state);
			
			void SetFrame(const Rect& frame) override;
			
		protected:
			void StateChanged(State state) override;
			
		private:
			void Initialize();
			
			bool ActivateImage(State state);
			bool ActivateTitle(State state);
			
			std::map<State, Image *> _images;
			std::map<State, String *> _titles;
			
			ImageView *_image;
			Label *_label;
			
			RNDefineMeta(Button, Control)
		};
	}
}

#endif /* __RAYNE_UIBUTTON_H__ */
