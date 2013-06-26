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
			if(_mesh)
				_mesh->Release();
			
			_text->Release();
			_font->Release();
		}
		
		void Label::Initialize()
		{
			_isDirty = false;
			
			_mesh = nullptr;
			_text = new String();
			
			_font  = nullptr;
			_alignment = TextAlignment::Left;
			
			SetFont(ResourcePool::SharedInstance()->ResourceWithName<Font>(kRNResourceKeyDefaultFont));
			SetTextColor(Color::White());
			SetInteractionEnabled(false);
			
			DrawMaterial()->SetShader(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyUITextShader));
		}
		
		void Label::SetFont(Font *font)
		{
			if(_font)
				_font->Release();
			
			_font    = font->Retain();
			_isDirty = true;
			
			DrawMaterial()->RemoveTextures();
			DrawMaterial()->AddTexture(_font->Texture());
		}
		
		void Label::SetText(String *text)
		{
			if(_text->IsEqual(text))
			{
				// Switch the text objects anyways, but don't mark it as dirty since we don't need to update the rendered text
				_text->Release();
				_text = text->Retain();
				return;
			}
			
			if(_text)
				_text->Release();
			
			_text = text ? text->Retain() : new String();
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
				if(_mesh)
				{
					_mesh->Release();
					_mesh = nullptr;
				}
				
				if(_text->Length() == 0)
				{
					_isDirty = false;
					return;
				}
				
				TextStyle style(Frame().Size());
				style.alignment = _alignment;
				
				_mesh = _font->RenderString(_text, style)->Retain();
				_isDirty = false;
			}
		}
		
		bool Label::Render(RenderingObject& object)
		{
			object.mesh = _mesh;
			return (_mesh != 0);
		}
	}
}
