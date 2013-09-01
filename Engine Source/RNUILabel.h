//
//  RNUILabel.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UILABEL_H__
#define __RAYNE_UILABEL_H__

#include "RNBase.h"
#include "RNColor.h"
#include "RNUIView.h"
#include "RNUIFont.h"
#include "RNUITypesetter.h"

namespace RN
{
	namespace UI
	{
		class Label : public View
		{
		public:
			Label();
			~Label();
			
			void SetText(String *text);
			void SetAttributedText(AttributedString *text);
			void SetTextColor(const Color& color);
			void SetFont(Font *font);
			void SetAlignment(TextAlignment alignment);
			void SetLineBreak(LineBreakMode mode);
			void SetNumberOfLines(uint32 lines);
			void SetFrame(const Rect& frame) override;
			
			String *Text() const { return _string->GetString(); }
			Color TextColor() const { return _color; }
			Font *TextFont() const { return _font; }
			TextAlignment Alignment() const { return _alignment; }
			LineBreakMode LineBreak() const { return _lineBreak; }
			uint32 NumberOfLines() const { return _lines; }
			
			Vector2 SizeThatFits() override;
			
		protected:
			void Update() override;
			void Draw(Renderer *renderer) override;
			
		private:
			void Initialize();
			
			TextAlignment _alignment;
			LineBreakMode _lineBreak;
			uint32 _lines;
			Font *_font;
			Color _color;
			
			Typesetter *_typesetter;
			AttributedString *_string;
			Model *_model;
			
			bool _isDirty;
			
			RNDefineMeta(Label, View)
		};
	}
}

#endif /* __RAYNE_UILABEL_H__ */
