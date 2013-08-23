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
			Initialize();
			
			Style *styleSheet = Style::GetSharedInstance();
			
			Dictionary *background = style->GetObjectForKey<Dictionary>(RNCSTR("background"));
			
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
		}
		
		TextField::~TextField()
		{
			_background->Release();
			_editor->Release();
		}
		
		
		
		void TextField::Initialize()
		{
			_background = new ImageView();
			_editor = new TextEditor(this);
			
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
			
			_background->SetFrame(Rect(0.0f, 0.0f, frame.width, frame.height));
			_editor->SetFrame(Rect(0.0f, 0.0f, frame.width, frame.height));
		}
	}
}
