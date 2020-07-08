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

		Label::Label() : _text(nullptr), _lines(nullptr), _font(nullptr), _alignment(Alignment::Left), _lineHeight(1.0f)
		{
			_internals->style.setAntiAlias(true);
		}
		Label::Label(const Rect &frame) : View(frame), _text(nullptr), _lines(nullptr), _font(nullptr), _alignment(Alignment::Left)
		{
			_internals->style.setAntiAlias(true);
		}
		Label::~Label()
		{
			
		}

		void Label::LayoutSubviews()
		{
            View::LayoutSubviews();
            
            _lineBounds.clear();
            _lines->Enumerate<RN::String>([&](RN::String *line, size_t index, bool &stop){
                
                SkRect textBounds;
                Data *data = line->GetDataWithEncoding(Encoding::UTF8);
                _font->_internals->font.measureText(data->GetBytes(), data->GetLength(), SkTextEncoding::kUTF8, &textBounds);
                _lineBounds.push_back(Vector2(textBounds.fRight, _font->_internals->font.getSize()));
                
            });
		}
    
        Vector2 Label::GetContentSize() const
        {
            if(!_font || !_lines || _lines->GetCount() == 0) return RN::Vector2();
            
            Vector2 contentSize;
            contentSize.y = _lines->GetCount() * _font->_internals->font.getSize() * _lineHeight;
            contentSize.y -= _font->_internals->font.getSize() * _lineHeight - _font->_internals->font.getSize(); //no additional padding below last line.
            
            _lines->Enumerate<RN::String>([&](RN::String *line, size_t index, bool &stop){
                
                SkRect textBounds;
                Data *data = line->GetDataWithEncoding(Encoding::UTF8);
                _font->_internals->font.measureText(data->GetBytes(), data->GetLength(), SkTextEncoding::kUTF8, &textBounds);
                if(textBounds.fRight > contentSize.x) contentSize.x = textBounds.fRight;
                
            });
            
            return contentSize;
        }
		
		void Label::SetText(String *text)
		{
            SafeRelease(_text);
            _text = text;
            SafeRetain(_text);
            
			SafeRelease(_lines);
            _lines = text->GetComponentsSeparatedByString(RNCSTR("\n"));
            SafeRetain(_lines);
			
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
			
			SetNeedsLayout();
		}
		
		void Label::SetAlignment(Alignment alignment)
		{
			_alignment = alignment;
		}
    
        void Label::SetLineHeight(float lineHeight)
        {
            _lineHeight = lineHeight;
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
