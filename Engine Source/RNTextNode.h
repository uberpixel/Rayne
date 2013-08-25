//
//  RNTextNode.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TEXTNODE_H__
#define __RAYNE_TEXTNODE_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNEntity.h"
#include "RNUITypesetter.h"

namespace RN
{
	class TextNode : public Entity
	{
	public:
		TextNode();
		~TextNode();
		
		void SetText(String *text);
		void SetAttributedText(AttributedString *text);
		void SetTextColor(const Color& color);
		void SetFont(UI::Font *font);
		void SetAlignment(UI::TextAlignment alignment);
		void SetLineBreak(UI::LineBreakMode mode);
		void SetNumberOfLines(uint32 lines);
		
		void Update(float delta) override;
		
	private:
		void Initialize();
		
		UI::TextAlignment _alignment;
		UI::LineBreakMode _lineBreak;
		uint32 _lines;
		UI::Font *_font;
		Color _color;
		
		UI::Typesetter *_typesetter;
		AttributedString *_string;
		
		bool _isDirty;
		
		RNDefineMeta(TextNode, Entity)
	};
}

#endif /* __RAYNE_TEXTNODE_H__ */
