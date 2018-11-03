//
//  RNUILabel.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUILabel.h"
#include "RNUIWindow.h"
#include "RNUIServer.h"
#include "RNUIInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Label, View)

		Label::Label() : _text(nullptr), _font(nullptr)
		{
			_internals->style.setTextEncoding(SkPaint::kUTF8_TextEncoding);
			_internals->style.setAntiAlias(true);
		}
		Label::Label(const Rect &frame) : View(frame), _text(nullptr), _font(nullptr)
		{
			_internals->style.setTextEncoding(SkPaint::kUTF8_TextEncoding);
			_internals->style.setAntiAlias(true);
		}
		Label::~Label()
		{
			
		}

		void Label::LayoutSubviews()
		{
			if(!_font || !_text) return;
			
			Rect rect = GetBounds();
			Data *data = _text->GetDataWithEncoding(Encoding::UTF8);
			/*__unused SkPoint end = */_font->_internals->shaper->shape(&_internals->builder, _internals->style, static_cast<char*>(data->GetBytes()), data->GetLength(), true, { 0, 0 }, rect.width); //Result could be used to center text or for dynamic label height
			_internals->textBlob = _internals->builder.make();
		}
		
		void Label::SetText(String *text)
		{
			SafeRelease(_text);
			_text = text;
			SafeRetain(_text);
		}
		
		void Label::SetColor(Color color)
		{
			_color = color;
			_internals->style.setColor(MakeColor(_color));
		}
		
		void Label::SetFont(Font *font)
		{
			SafeRelease(_font);
			_font = font;
			SafeRetain(_font);
			
			_internals->style.setTypeface(_font->_internals->typeface);
			_internals->style.setTextSize(_font->GetSize());
		}
		
		void Label::SetAlignment(Alignment alignment)
		{
			_alignment = alignment;
			
			SkPaint::Align align = SkPaint::kLeft_Align;
			switch(_alignment)
			{
				case Alignment::Left:
					align = SkPaint::kLeft_Align;
					break;
					
				case Alignment::Center:
					align = SkPaint::kCenter_Align;
					break;
					
				case Alignment::Right:
					align = SkPaint::kRight_Align;
					break;
			}
			
			_internals->style.setTextAlign(align);
		}
		

		// ---------------------
		// MARK: -
		// MARK: Drawing
		// ---------------------

		void Label::Draw(Context *context) const
		{
			View::Draw(context);
			
			if(_text && _text->GetLength() > 0)
			{
				if(_color.a > 0.05)
				{
					context->DrawLabel(this);
				}
			}
		}
	}
}
