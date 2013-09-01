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
		class TextEditorInterface
		{
		public:
			virtual void SelectionDidChange(TextEditor *editor, const Range& selection) {}
		};
		
		class TextEditor : public View
		{
		public:
			TextEditor(TextEditorInterface *interface);
			~TextEditor() override;
			
			void SetFrame(const Rect& frame) override;
			void SetTypingAttributes(Dictionary *attributes);
			void SetSelection(const Range& selection);
			
			void ProcessEvent(Event *event);
			
		protected:
			void Draw(Renderer *renderer) override;
			void Update() override;
			
		private:
			void InsertString(String *string);
			
			TextEditorInterface *_interface;
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
