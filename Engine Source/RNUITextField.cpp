//
//  RNUITextField.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITextField.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(TextField)
		
		TextField::TextField(Dictionary *style)
		{
			RN_ASSERT(style, "Style mustn't be NULL!");
			
			Initialize();
			
			Style *styleSheet = Style::GetSharedInstance();
			Texture *texture = styleSheet->GetTextureWithName(style->GetObjectForKey<String>(RNCSTR("texture")));
			Dictionary *contentInsets = style->GetObjectForKey<Dictionary>(RNCSTR("contentInsets"));
			Dictionary *clipInsets = style->GetObjectForKey<Dictionary>(RNCSTR("clipInsets"));
			Dictionary *tInsets = style->GetObjectForKey<Dictionary>(RNCSTR("insets"));
			
			Array *states = style->GetObjectForKey<Array>(RNCSTR("states"));
			
			states->Enumerate<Dictionary>([&](Dictionary *state, size_t index, bool &stop) {
				
				String *name = state->GetObjectForKey<String>(RNCSTR("name"));
				Dictionary *atlas  = state->GetObjectForKey<Dictionary>(RNCSTR("atlas"));
				Dictionary *insets = state->GetObjectForKey<Dictionary>(RNCSTR("insets"));
				
				if(!insets)
					insets = tInsets;
				
				Image *image = new Image(texture);
				
				if(atlas)
					image->SetAtlas(Style::ParseAtlas(atlas), false);
				
				if(insets)
					image->SetEdgeInsets(Style::ParseEdgeInsets(insets));
				
				_backgroundImages.SetValueForState(image->Autorelease(), Style::ParseState(name));
			});
			
			_contentInsets = Style::ParseEdgeInsets(contentInsets);
			_background->SetClipInsets(Style::ParseEdgeInsets(clipInsets));
			
			StateChanged(GetState());
		}
		
		TextField::~TextField()
		{
			_background->Release();
			_editor->Release();
		}
		
		TextField *TextField::WithType(Type type)
		{
			Dictionary *dictionary = nullptr;
			Style *styleSheet = Style::GetSharedInstance();
			
			switch(type)
			{
				case Type::RoundedRect:
					dictionary = styleSheet->GetTextfieldStyleWithKeyPath(RNCSTR("textfields.RNRoundedRect"));
					break;
					
				case Type::Bezel:
					dictionary = styleSheet->GetTextfieldStyleWithKeyPath(RNCSTR("textfields.RNBezel"));
					break;
			}
			
			TextField *textField = new TextField(dictionary);
			return textField->Autorelease();
		}
		
		
		
		void TextField::Initialize()
		{
			_background = new ImageView();
			_editor = new TextEditor();
			_editor->SetDelegate(this);
			_editor->GetTypesetter()->SetLineBreak(LineBreakMode::None);
			
			_formatter = nullptr;
			_delegate = nullptr;
			
			AddSubview(_background);
			
			_background->AddSubview(_editor);
			_background->SetClipSubviews(true);
		}
		
		
		void TextField::StateChanged(State state)
		{
			_background->SetImage(_backgroundImages.GetValueForState(state));
			SetNeedsLayoutUpdate();
		}
		
		
		
		void TextField::SetDelegate(TextFieldDelegate *delegate)
		{
			_delegate = delegate;
		}
		
		void TextField::SetText(String *text)
		{
			_editor->SetText(text);
		}
		
		void TextField::SetAttributedText(AttributedString *string)
		{
			_editor->SetAttributedText(string);
		}
		
		void TextField::SetValue(Object *object)
		{
			RN_ASSERT(_formatter, "SetValue() requires a formatter to be set!");
			
			AttributedString *temp = _formatter->GetAttributedStringForObject(object, _editor->GetTypingAttributes());
			SetAttributedText(temp);
		}
		
		void TextField::SetFormatter(Formatter *formatter)
		{
			if(_formatter)
				_formatter->Release();
			
			_formatter = formatter->Retain();
		}
		
		Object *TextField::GetValue() const
		{
			RN_ASSERT(_formatter, "Getvalue() requires a formatter to be set!");
			return _formatter->GetObjectForString(_editor->GetText());
		}
		
		
		void TextField::SetValueForUndefinedKey(Object *value, const std::string& key)
		{
			if(_formatter && key == "value")
			{
				SetValue(value);
				return;
			}
			
			Control::SetValueForUndefinedKey(value, key);
		}
		
		Object *TextField::GetValueForUndefinedKey(const std::string& key)
		{
			if(_formatter && key == "value")
			{
				return GetValue();
			}
			
			return Control::GetValueForUndefinedKey(key);
		}
		
		
		
		bool TextField::TextEditorShouldReturn(TextEditor *editor)
		{
			if(_delegate && !_delegate->TextFieldShouldEndEditing(this))
				return false;
			
			try
			{
				Object *temp = _formatter->GetObjectForString(_editor->GetText());
				if(temp)
				{
					SetValue(temp);
					return true;
				}
			}
			catch(Exception e)
			{}
			
			return false;
		}
		
		bool TextField::CanBecomeFirstResponder()
		{
			if(_delegate)
				return _delegate->TextFieldShouldBeginEditing(this);
				
			return true;
		}
		
		void TextField::BecomeFirstResponder()
		{
			Control::BecomeFirstResponder();
			
			if(_delegate)
				_delegate->TextFieldDidBeginEditing(this);
		}
		
		void TextField::ResignFirstResponder()
		{
			Control::ResignFirstResponder();
			
			if(_delegate)
				_delegate->TextFieldDidEndEditing(this);
		}
		
		
		
		void TextField::KeyDown(Event *event)
		{
			_editor->ProcessEvent(event);
		}
		
		void TextField::KeyRepeat(Event *event)
		{
			_editor->ProcessEvent(event);
		}
		
		void TextField::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect frame = GetFrame();
			Vector2 size = _editor->GetTypesetter()->GetDimensions();
			
			Rect editorRect;
			
			editorRect.x = _contentInsets.left;
			editorRect.y = roundf((frame.height * 0.5f) - (size.y * 0.5));
			editorRect.width  = frame.width - (_contentInsets.right + _contentInsets.left);
			editorRect.height = size.y;
			
			_background->SetFrame(Rect(0.0f, 0.0f, frame.width, frame.height));
			_editor->SetFrame(editorRect);
		}
	}
}
