//
//  RNUILabel.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUILabel.h"
#include "RNResourcePool.h"

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
		}
		
		void Label::Initialize()
		{
			_isDirty = false;
			
			_font      = nullptr;
			_model     = nullptr;
			
			_alignment = TextAlignment::Left;
			_string    = new AttributedString(RNSTR(""));
			
			SetFont(ResourcePool::SharedInstance()->ResourceWithName<Font>(kRNResourceKeyDefaultFont));
			SetTextColor(Color::White());
			SetInteractionEnabled(false);
		}
		
		void Label::SetFont(Font *font)
		{
			if(_font)
				_font->Release();
			
			_font    = font->Retain();
			_isDirty = true;

			if(_string)
			{
				String *text = _string->String()->Retain();
				SetText(text);
				text->Release();
			}
		}
		
		void Label::SetText(String *text)
		{
			RN_ASSERT(text, "Text mustn't be NULL!");
			
			if(_string)
			{
				_string->Release();
				_string = nullptr;
			}
			
			_string = new AttributedString(text);
			_string->AddAttribute(kRNTypesetterFontAttribute, _font, Range(0, text->Length()));
			
			_isDirty = true;
		}
		
		void Label::SetTextColor(const Color& color)
		{
			_color = color;
			DrawMaterial()->ambient = _color;
		}
		
		void Label::SetAlignment(TextAlignment alignment)
		{
			_alignment = alignment;
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
				
				Typesetter typesetter(_string, Bounds());
				
				typesetter.SetMaximumLines(1);
				typesetter.SetLineBreak(LineBreakMode::TruncateMiddle);
				typesetter.SetAlignment(_alignment);
				
				_model = typesetter.LineModel()->Retain();
				_isDirty = false;
			}
		}
		
		void Label::Render(Renderer *renderer)
		{
			Update();
			
			if(_model)
			{
				RenderingObject object;
				
				object.transform = &_finalTransform;
				
				uint32 count = _model->Meshes(0);
				for(uint32 i=0; i<count; i++)
				{
					object.mesh     = _model->MeshAtIndex(0, i);
					object.material = _model->MaterialAtIndex(0, i);
					
					renderer->RenderObject(object);
				}
			}
			
			RenderChilds(renderer);
		}
	}
}
