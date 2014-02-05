//
//  RNUILabel.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UILABEL_H__
#define __RAYNE_UILABEL_H__

#include "RNBase.h"
#include "RNColor.h"
#include "RNUIView.h"
#include "RNUIFont.h"
#include "RNUITypesetter.h"
#include "RNUIColor.h"

namespace RN
{
	namespace UI
	{
		class Label : public View
		{
		public:
			RNAPI Label();
			RNAPI ~Label();
			
			RNAPI void SetText(String *text);
			RNAPI void SetAttributedText(AttributedString *text);
			RNAPI void SetTextColor(const RN::Color& color);
			RNAPI void SetTextColor(Color *color);
			RNAPI void SetFont(Font *font);
			RNAPI void SetAlignment(TextAlignment alignment);
			RNAPI void SetLineBreak(LineBreakMode mode);
			RNAPI void SetNumberOfLines(uint32 lines);
			RNAPI void SetFrame(const Rect& frame) override;
			
			RNAPI String *GetText() const { return _string->GetString(); }
			RNAPI Color *GetTextColor() const { return _color; }
			RNAPI Font *GetFont() const { return _font; }
			RNAPI TextAlignment GetAlignment() const { return _alignment; }
			RNAPI LineBreakMode GetLineBreak() const { return _lineBreak; }
			RNAPI uint32 GetNumberOfLines() const { return _lines; }
			
			RNAPI Vector2 GetSizeThatFits() override;
			
		protected:
			RNAPI void Update() override;
			RNAPI void Draw(Renderer *renderer) override;
			
		private:
			void Initialize();
			
			TextAlignment _alignment;
			LineBreakMode _lineBreak;
			uint32 _lines;
			Font *_font;
			Color *_color;
			
			Typesetter *_typesetter;
			AttributedString *_string;
			Model *_model;
			
			bool _isDirty;
			
			RNDeclareMeta(Label, View)
		};
	}
}

#endif /* __RAYNE_UILABEL_H__ */
