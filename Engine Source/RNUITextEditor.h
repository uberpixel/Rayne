//
//  RNUITextEditor.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UITEXTEDITOR_H__
#define __RAYNE_UITEXTEDITOR_H__

#include "RNBase.h"
#include "RNAttributedString.h"
#include "RNUITypesetter.h"
#include "RNUIView.h"

namespace RN
{
	namespace UI
	{
		class TextEditor;
		class TextEditorDelegate
		{
		public:
			RNAPI virtual void TextEditorSelectionDidChange(TextEditor *editor, const Range &selection) {}
			RNAPI virtual bool TextEditorShouldReturn(TextEditor *editor) { return true; }
		};
		
		class TextEditor : public View
		{
		public:
			RNAPI TextEditor();
			RNAPI ~TextEditor() override;
			
			RNAPI void SetFrame(const Rect &frame) override;
			RNAPI void SetTypingAttributes(Dictionary *attributes);
			RNAPI void SetSelection(const Range &selection);
			RNAPI void SetText(String *text);
			RNAPI void SetAttributedText(AttributedString *text);
			RNAPI void SetDelegate(TextEditorDelegate *delegate);
			
			RNAPI void ProcessEvent(Event *event);
			
			RNAPI String *GetText() const { return _string->GetString(); }
			RNAPI Dictionary *GetTypingAttributes() const { return _typingAttributes; }
			RNAPI AttributedString *GetAttributedText() const { return _string; }
			RNAPI Typesetter *GetTypesetter() const { return _typesetter; }
			
		protected:
			RNAPI void Draw(Renderer *renderer) override;
			RNAPI void Update() override;
			
		private:
			void InsertString(String *string);
			
			TextEditorDelegate *_delegate;
			Typesetter *_typesetter;
			
			bool _isDirty;
			Model *_model;
			
			AttributedString *_string;
			Dictionary *_typingAttributes;
			
			Range _selection;
			
			RNDeclareMeta(TextEditor)
		};
	}
}

#endif /* __RAYNE_UITEXTEDITOR_H__ */
