//
//  RNTextNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTextNode.h"
#include "RNUIStyle.h"

namespace RN
{
	RNDeclareMeta(TextNode)
	
	TextNode::TextNode()
	{
		Initialize();
	}
	
	TextNode::~TextNode()
	{
		_string->Release();
		_font->Release();
		
		delete _typesetter;
	}
	
	void TextNode::Initialize()
	{
		_isDirty = false;
		_font = nullptr;
		
		_string     = new AttributedString(RNCSTR(""));
		_typesetter = new UI::Typesetter(_string, Rect(0.0f, 0.0f, 512.0f, 512.0f));
		_typesetter->SetAllowPartiallyClippedLined(false);
		
		SetFont(UI::Style::GetSharedInstance()->GetFont(UI::Style::FontStyle::DefaultFont));
		SetTextColor(Color::White());
		
		SetAlignment(UI::TextAlignment::Left);
		SetLineBreak(UI::LineBreakMode::TruncateTail);
		SetNumberOfLines(1);
		
		SetScale(0.01f);
	}
	
	void TextNode::SetFont(UI::Font *font)
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
	
	void TextNode::SetText(String *text)
	{
		RN_ASSERT(text, "Text mustn't be NULL!");
		
		if(_string)
			_string->Release();
		
		_string = new AttributedString(text);
		_string->AddAttribute(kRNTypesetterFontAttribute, _font, Range(0, text->GetLength()));
		
		_typesetter->SetText(_string);
		_isDirty = true;
	}
	
	void TextNode::SetAttributedText(AttributedString *text)
	{
		RN_ASSERT(text, "Text mustn't be NULL!");
		
		if(_string)
			_string->Release();
		
		_string  = text->Retain();
		_isDirty = true;
		
		_typesetter->SetText(_string);
	}
	
	void TextNode::SetTextColor(const Color& color)
	{
		_color = color;
	}
	
	void TextNode::SetAlignment(UI::TextAlignment alignment)
	{
		_alignment = alignment;
		_typesetter->SetAlignment(_alignment);
		_isDirty = true;
	}
	
	void TextNode::SetLineBreak(UI::LineBreakMode mode)
	{
		_lineBreak = mode;
		_typesetter->SetLineBreak(_lineBreak);
		_isDirty = true;
	}
	
	void TextNode::SetNumberOfLines(uint32 lines)
	{
		_lines = lines;
		_typesetter->SetMaximumLines(_lines);
		_isDirty = true;
	}
	
	
	void TextNode::Update(float delta)
	{
		Entity::Update(delta);
		
		if(_isDirty)
		{
			_typesetter->SetFrame(Rect(Vector2(0.0f), _typesetter->GetDimensions()));
			SetModel(_typesetter->GetLineModel());
			
			Model *model = GetModel();
			size_t count = model->GetMeshCount(0);
			
			for(size_t i = 0; i < count; i ++)
			{
				Material *material = model->GetMaterialAtIndex(0, i);
				material->depthtest = true;
				material->depthwrite = true;
				material->blending = true;
				material->culling = false;
			}
			
			_isDirty = false;
		}
	}
}
