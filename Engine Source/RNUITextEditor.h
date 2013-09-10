//
//  RNUITextEditor.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
			virtual void TextEditorSelectionDidChange(TextEditor *editor, const Range& selection) {}
			virtual bool TextEditorShouldReturn(TextEditor *editor) { return true; }
		};
		
		class TextEditor : public View
		{
		public:
			TextEditor();
			~TextEditor() override;
			
			void SetFrame(const Rect& frame) override;
			void SetTypingAttributes(Dictionary *attributes);
			void SetSelection(const Range& selection);
			void SetText(String *text);
			void SetDelegate(TextEditorDelegate *delegate);
			
			void ProcessEvent(Event *event);
			
			String *GetText() const { return _string->GetString(); }
			
		protected:
			void Draw(Renderer *renderer) override;
			void Update() override;
			
		private:
			void InsertString(String *string);
			
			TextEditorDelegate *_delegate;
			Typesetter *_typesetter;
			
			bool _isDirty;
			Model *_model;
			
			AttributedString *_string;
			Dictionary *_typingAttributes;
			
			Range _selection;
			
			RNDefineMeta(TextEditor, View)
		};
	}
}

#endif /* __RAYNE_UITEXTEDITOR_H__ */
