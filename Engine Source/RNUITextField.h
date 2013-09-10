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
#include "RNFormatter.h"
#include "RNUIControl.h"
#include "RNUIImageView.h"
#include "RNUITextEditor.h"

namespace RN
{
	namespace UI
	{
		class TextField : public Control, TextEditorDelegate
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
			
			void SetText(String *text);
			void SetAttributedText(AttributedString *string);
			void SetFormatter(Formatter *formatter);
			void SetValue(Object *value);
			
			String *GetText() const { return _editor->GetText(); }
			AttributedString *GetAttributedText() const { return _editor->GetAttributedText(); }
			Object *GetValue() const;
			
			void KeyDown(Event *event) override;
			void KeyRepeat(Event *event) override;
			
			bool CanBecomeFirstResponder() const override;
			
		protected:
			void LayoutSubviews() override;
			bool TextEditorShouldReturn(TextEditor *editor) override;
			
			void SetValueForUndefinedKey(const std::string& key, Object *value) override;
			Object *GetValueForUndefinedKey(const std::string& key) override;
			
		private:
			void Initialize();
			
			ImageView *_background;
			TextEditor *_editor;
			Formatter *_formatter;
			EdgeInsets _contentInsets;
			
			RNDefineMeta(TextField, Control)
		};
	}
}

#endif /* __RAYNE_UITEXTFIELD_H__ */
