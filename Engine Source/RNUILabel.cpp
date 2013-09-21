//
//  RNUILabel.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUILabel.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Label)
		
		Label::Label()
		{
			Initialize();
		}
		
		Label::~Label()
		{
			_string->Release();
			_font->Release();
			
			delete _typesetter;
		}
		
		void Label::Initialize()
		{
			_isDirty = false;
			
			_font      = nullptr;
			_model     = nullptr;
			_color     = new Color(RN::Color::White());
			
			_string     = new AttributedString(RNCSTR(""));
			_typesetter = new Typesetter(_string, GetBounds());
			_typesetter->SetAllowPartiallyClippedLined(false);
			
			SetInteractionEnabled(false);
			SetBackgroundColor(RN::Color::ClearColor());
			SetFont(Style::GetSharedInstance()->GetFont(UI::Style::FontStyle::DefaultFont));
			SetTextColor(Style::GetSharedInstance()->GetColor(Style::ColorStyle::TextColor));
			
			SetAlignment(TextAlignment::Left);
			SetLineBreak(LineBreakMode::TruncateTail);
			SetNumberOfLines(1);
		}
		
		void Label::SetFont(Font *font)
		{
			RN_ASSERT(font, "Font mustn't be NULL!");
			
			if(_font)
				_font->Release();
			
			_font    = font->Retain();
			_isDirty = true;

			if(_string)
			{
				String *text = _string->GetString()->Retain();
				SetText(text);
				text->Release();
			}
		}
		
		void Label::SetText(String *text)
		{
			RN_ASSERT(text, "Text mustn't be NULL!");
			
			if(_string)
				_string->Release();
			
			_string = new AttributedString(text);
			_string->AddAttribute(kRNTypesetterFontAttribute, _font, Range(0, text->GetLength()));
			_string->AddAttribute(kRNTypesetterColorAttribute, _color, Range(0, text->GetLength()));
			
			_typesetter->SetText(_string);
			_isDirty = true;
		}
		
		void Label::SetAttributedText(AttributedString *text)
		{
			RN_ASSERT(text, "Text mustn't be NULL!");
			
			if(_string)
				_string->Release();
			
			_string  = text->Retain();
			_isDirty = true;
			
			_typesetter->SetText(_string);
		}
		
		void Label::SetTextColor(const RN::Color& color)
		{
			SetTextColor(Color::WithRNColor(color));
			
		}
		
		void Label::SetTextColor(Color *color)
		{
			_color->Release();
			_color = color->Retain();
			
			Range range(0, _string->GetLength());
			
			_string->RemoveAttribute(kRNTypesetterColorAttribute, range);
			_string->AddAttribute(kRNTypesetterColorAttribute, _color, range);
			
			_typesetter->InvalidateStringInRange(range);
			_isDirty = true;
		}
		
		void Label::SetAlignment(TextAlignment alignment)
		{
			_alignment = alignment;
			_typesetter->SetAlignment(_alignment);
			_isDirty = true;
		}
		
		void Label::SetLineBreak(LineBreakMode mode)
		{
			_lineBreak = mode;
			_typesetter->SetLineBreak(_lineBreak);
			_isDirty = true;
		}
		
		void Label::SetNumberOfLines(uint32 lines)
		{
			_lines = lines;
			_typesetter->SetMaximumLines(_lines);
			_isDirty = true;
		}
		
		Vector2 Label::GetSizeThatFits()
		{
			return _typesetter->GetDimensions();
		}
		
		void Label::SetFrame(const Rect& frame)
		{
			View::SetFrame(frame);
			
			_typesetter->SetFrame(GetBounds());
			_isDirty = true;
		}
		
		void Label::Update()
		{
			View::Update();
			
			if(_isDirty)
			{
				if(_model)
				{
					_model->Release();
					_model = nullptr;
				}
				
				if(!_string)
					return;
				
				_model = _typesetter->GetLineModel()->Retain();
				_isDirty = false;
			}
		}
		
		void Label::Draw(Renderer *renderer)
		{
			View::Draw(renderer);
			
			if(_model)
			{
				RenderingObject object;
				PopulateRenderingObject(object);
				
				uint32 count = _model->GetMeshCount(0);
				for(uint32 i=0; i<count; i++)
				{
					object.mesh     = _model->GetMeshAtIndex(0, i);
					object.material = _model->GetMaterialAtIndex(0, i);
					
					renderer->RenderObject(object);
				}
			}
		}
	}
}
