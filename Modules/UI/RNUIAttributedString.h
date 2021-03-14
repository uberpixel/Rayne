//
//  RNUIAttributedString.h
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_UIATTRIBUTEDSTRING_H_
#define __RAYNE_UIATTRIBUTEDSTRING_H_

#include "RNUIConfig.h"
#include "RNUIFontManager.h"

namespace RN
{
	namespace UI
	{
		class AttributedString;
	
		enum TextWrapMode
		{
			TextWrapModeNone,
			TextWrapModeCharacter,
			TextWrapModeWord
		};
		
		enum TextAlignment
		{
			TextAlignmentLeft,
			TextAlignmentRight,
			TextAlignmentCenter
		};
	
		enum TextVerticalAlignment
		{
			TextVerticalAlignmentTop,
			TextVerticalAlignmentCenter,
			TextVerticalAlignmentBottom
		};
	
		class TextAttributes
		{
		friend AttributedString;
		public:
			UIAPI TextAttributes(Font *font, float fontSize, RN::Color color, TextAlignment alignment = TextAlignmentLeft, TextWrapMode wrapMode = TextWrapModeWord, float kerning = 0.0f) : _font(font->Retain()), _color(color), _alignment(alignment), _wrapMode(wrapMode), _fontSize(fontSize), _kerning(kerning) {}
			UIAPI TextAttributes(const TextAttributes &attributes) : _font(attributes._font->Retain()), _color(attributes._color), _alignment(attributes._alignment), _wrapMode(attributes._wrapMode), _fontSize(attributes._fontSize), _kerning(attributes._kerning), _range(attributes._range) {}
			UIAPI ~TextAttributes() {_font->Release();}
			
			UIAPI void SetFont(Font *font);
			UIAPI void SetFontSize(float size);
			UIAPI void SetColor(const Color &color);
			UIAPI void SetAlignment(TextAlignment alignment);
			UIAPI void SetWrapMode(TextWrapMode wrapMode);
			UIAPI void SetKerning(float kerning);
			
			Font *GetFont() const { return _font; }
			const Color &GetColor() const { return _color; }
			TextAlignment GetAlignment() const { return _alignment; }
			TextWrapMode GetWrapMode() const { return _wrapMode; }
			float GetFontSize() const { return _fontSize; }
			float GetKerning() const { return _kerning; }
			
		private:
			Font *_font;
			RN::Color _color;
			TextAlignment _alignment;
			TextWrapMode _wrapMode;
			float _fontSize;
			float _kerning;
			
			RN::Range _range;
		};
	
		class AttributedString : public String
		{
		public:
			UIAPI AttributedString(String *string);
			UIAPI AttributedString(const AttributedString *string);
			UIAPI ~AttributedString();
			
			UIAPI void SetAttributes(const TextAttributes &attributes, const RN::Range &range);
			UIAPI const TextAttributes *GetAttributesAtIndex(size_t index) const;

		private:
			std::vector<TextAttributes> _attributes;

			RNDeclareMetaAPI(AttributedString, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIATTRIBUTEDSTRING_H_ */
