//
//  RNUITextField.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		class TextField;
		class TextFieldDelegate
		{
		public:
			RNAPI virtual bool TextFieldShouldBeginEditing(TextField *textField) { return true; }
			RNAPI virtual void TextFieldDidBeginEditing(TextField *textField) {}
			RNAPI virtual bool TextFieldShouldEndEditing(TextField *textField) { return true; }
			RNAPI virtual void TextFieldDidEndEditing(TextField *textField) {}
		};
		
		class TextField : public Control, TextEditorDelegate
		{
		public:
			enum class Type
			{
				RoundedRect,
				Bezel
			};
			
			RNAPI TextField(Dictionary *style);
			RNAPI ~TextField() override;
			
			RNAPI static TextField *WithType(Type type);
			
			RNAPI void SetText(String *text);
			RNAPI void SetAttributedText(AttributedString *string);
			RNAPI void SetFormatter(Formatter *formatter);
			RNAPI void SetValue(Object *value);
			RNAPI void SetDelegate(TextFieldDelegate *delegate);
			
			RNAPI String *GetText() const { return _editor->GetText(); }
			RNAPI AttributedString *GetAttributedText() const { return _editor->GetAttributedText(); }
			RNAPI Object *GetValue() const;
			
			RNAPI void KeyDown(Event *event) override;
			RNAPI void KeyRepeat(Event *event) override;
			
			RNAPI bool CanBecomeFirstResponder() override;
			RNAPI void BecomeFirstResponder() override;
			RNAPI void ResignFirstResponder() override;
			RNAPI void StateChanged(State state) override;
			
		protected:
			RNAPI void LayoutSubviews() override;
			RNAPI bool TextEditorShouldReturn(TextEditor *editor) override;
			
			RNAPI void SetValueForUndefinedKey(const std::string& key, Object *value) override;
			RNAPI Object *GetValueForUndefinedKey(const std::string& key) override;
			
		private:
			void Initialize();
			
			TextFieldDelegate *_delegate;
			
			ControlStateStore<Image> _backgroundImages;
			
			ImageView *_background;
			TextEditor *_editor;
			Formatter *_formatter;
			EdgeInsets _contentInsets;
			
			RNDefineMeta(TextField, Control)
		};
	}
}

#endif /* __RAYNE_UITEXTFIELD_H__ */
