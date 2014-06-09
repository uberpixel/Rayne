//
//  RNUITextEditor.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITextEditor.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(TextEditor, View)
		
		TextEditor::TextEditor()
		{
			_string = new AttributedString(RNCSTR(""));
			_typingAttributes = new Dictionary();
			_selection = Range(0, 0);
			_typesetter = new Typesetter(_string, GetFrame());
			_isDirty = true;
			
			_model     = nullptr;
			_delegate  = nullptr;
			
			SetBackgroundColor(Color::ClearColor());
		}
		
		TextEditor::~TextEditor()
		{
			_string->Release();
			_typingAttributes->Release();
		}
		
		
		
		void TextEditor::SetDelegate(TextEditorDelegate *delegate)
		{
			_delegate = delegate;
		}
		
		void TextEditor::SetFrame(const Rect &frame)
		{
			View::SetFrame(frame);
			_typesetter->SetFrame(frame);
			_isDirty = true;
		}
		
		void TextEditor::SetTypingAttributes(Dictionary *attributes)
		{
			_typingAttributes->Release();
			_typingAttributes = attributes->Retain();
		}
		
		void TextEditor::SetSelection(const Range &selection)
		{
			_selection = selection;
			SetTypingAttributes(_string->GetAttributesAtIndex(_selection.origin));
		}
		
		void TextEditor::SetText(String *text)
		{
			AttributedString *temp = new AttributedString(text);
			SetAttributedText(temp);
			
			temp->Release();
		}
		
		void TextEditor::SetAttributedText(AttributedString *text)
		{
			_string->Release();
			_string = text->Retain();
			
			_typesetter->SetText(_string);
			
			_isDirty = true;
			_selection = Range(_string->GetLength(), 0);
		}
		
		
		
		void TextEditor::InsertString(String *string)
		{
			if(string)
			{
				_string->ReplaceCharacters(string, _selection, _typingAttributes);
				_typesetter->InvalidateStringInRange(_selection);
				
				_selection.origin ++;
				_selection.length = 0;
			}
			else
			{
				if(_selection.origin == 0)
					return;
				
				_selection.origin --;
				_selection.length = 1;
				
				_string->ReplaceCharacters(string, _selection, _typingAttributes);
				_typesetter->InvalidateStringInRange(_selection);
				
				_selection.length = 0;
			}
			
			_isDirty = true;
			
			if(_delegate)
				_delegate->TextEditorSelectionDidChange(this, _selection);
		}
		
		void TextEditor::ProcessEvent(Event *event)
		{
			switch(event->GetType())
			{
				case Event::Type::KeyDown:
				case Event::Type::KeyRepeat:
				{
					switch(event->GetCode())
					{
						case KeyDelete:
							InsertString(nullptr);
							break;
							
						case KeyReturn:
							if(_delegate && _delegate->TextEditorShouldReturn(this))
								GetWidget()->MakeFirstResponder(nullptr);

							break;
							
						default:
						{
							if(CodePoint(event->GetCode()).IsPrintable())
							{
								UniChar unicode[2];
								unicode[0] = event->GetCode();
								unicode[1] = 0;
								
								InsertString(String::WithBytes(unicode, Encoding::UTF32));
							}
							break;
						}
					}
					
					break;
				}
					
				default:
					break;
			}
		}
		
		
		
		void TextEditor::Update()
		{
			View::Update();
			
			if(_isDirty)
			{
				if(_model)
				{
					_model->Release();
					_model = nullptr;
				}
				
				_model   = _typesetter->GetLineModel()->Retain();
				_isDirty = false;
			}
		}
		
		void TextEditor::Draw(Renderer *renderer)
		{
			View::Draw(renderer);
			
			if(_model)
			{
				RenderingObject object;
				PopulateRenderingObject(object);
				
				size_t count = _model->GetMeshCount(0);
				for(size_t i = 0; i < count; i ++)
				{
					object.mesh     = _model->GetMeshAtIndex(0, i);
					object.material = _model->GetMaterialAtIndex(0, i);
					
					renderer->RenderObject(object);
				}
			}
		}
	}
}
