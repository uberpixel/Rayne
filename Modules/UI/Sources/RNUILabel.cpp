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
			_font->_internals->shaper->shape(&_internals->builder, _internals->style, static_cast<char*>(data->GetBytes()), data->GetLength(), true, { 0, 0 }, rect.width);
			_internals->textBlob = _internals->builder.make();
			
			SkRect textBounds = _internals->textBlob->bounds();
			_contentSize = Vector2(textBounds.width(), textBounds.height());
			
			if(_alignment == Alignment::Left)
			{
				rect.x = 0.0f;
			}
			else if(_alignment == Alignment::Center)
			{
				rect.x = (rect.width - textBounds.width()) / 2;
			}
			else if(_alignment == Alignment::Right)
			{
				rect.x = rect.width - textBounds.width();
			}
			
			SetBounds(rect);
		}
		
		void Label::SetText(String *text)
		{
			SafeRelease(_text);
			_text = text;
			SafeRetain(_text);
			
			SetNeedsLayout();
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
			
			SetNeedsLayout();
		}
		
		void Label::SetAlignment(Alignment alignment)
		{
			_alignment = alignment;
			SetNeedsLayout();
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
