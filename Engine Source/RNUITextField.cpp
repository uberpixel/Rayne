//
//  RNUITextField.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUITextField.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(TextField)
		
		TextField::TextField(Dictionary *style)
		{
			RN_ASSERT(style, "Style mustn't be NULL!");
			
			Initialize();
			
			Style *styleSheet = Style::GetSharedInstance();
			Dictionary *background = style->GetObjectForKey<Dictionary>(RNCSTR("background"));
			Dictionary *insets = style->GetObjectForKey<Dictionary>(RNCSTR("contentInsets"));
			
			if(background)
			{
				Atlas atlas = Style::ParseAtlas(background->GetObjectForKey<Dictionary>(RNCSTR("atlas")));
				EdgeInsets insets = Style::ParseEdgeInsets(background->GetObjectForKey<Dictionary>(RNCSTR("insets")));
				Texture *texture = styleSheet->TextureWithName(background->GetObjectForKey<String>(RNCSTR("texture")));
				
				Image *image = new Image(texture);
				image->SetAtlas(atlas, false);
				image->SetEdgeInsets(insets);
				
				_background->SetImage(image);
				
				image->Release();
			}
			
			_contentInsets = Style::ParseEdgeInsets(insets);
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
					dictionary = styleSheet->TextfieldStyle(RNCSTR("RNRoundedRect"));
					break;
					
				case Type::Bezel:
					dictionary = styleSheet->TextfieldStyle(RNCSTR("RNBezel"));
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
			
			AddSubview(_background);
			AddSubview(_editor);
			
			SetClipSubviews(true);
		}
		
		
		void TextField::KeyDown(Event *event)
		{
			_editor->ProcessEvent(event);
		}
		
		void TextField::KeyRepeat(Event *event)
		{
			_editor->ProcessEvent(event);
		}
		
		
		
		bool TextField::CanBecomeFirstResponder() const
		{
			return true;
		}
		
		
		void TextField::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect frame = Frame();
			Rect editorRect = Rect(_contentInsets.left, _contentInsets.top, frame.width - _contentInsets.right, frame.height - _contentInsets.bottom);
			
			_background->SetFrame(Rect(0.0f, 0.0f, frame.width, frame.height));
			_editor->SetFrame(editorRect);
		}
	}
}
