//
//  RNUITextField.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UITEXTFIELD_H__
#define __RAYNE_UITEXTFIELD_H__

#include "RNBase.h"
#include "RNDictionary.h"
#include "RNUIControl.h"
#include "RNUIImageView.h"
#include "RNUITextEditor.h"

namespace RN
{
	namespace UI
	{
		class TextField : public Control, TextEditorInterface
		{
		public:
			enum class Type
			{
				RoundedRect,
				Bezel
			};
			
			TextField(Dictionary *style);
			~TextField() override;
			
			static TextField *WithType(Type type);
			
			void KeyDown(Event *event) override;
			void KeyRepeat(Event *event) override;
			
			bool CanBecomeFirstResponder() const override;
			
		protected:
			void LayoutSubviews() override;
			
		private:
			void Initialize();
			
			ImageView *_background;
			TextEditor *_editor;
			
			RNDefineMeta(TextField, Control)
		};
	}
}

#endif /* __RAYNE_UITEXTFIELD_H__ */
