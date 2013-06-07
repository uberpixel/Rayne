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
			
			SetFont(ResourcePool::SharedInstance()->ResourceWithName<Font>(kRNResourceKeyDefaultFont));
			SetTextColor(Color::White());
			
			DrawMaterial()->SetShader(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyUITextShader));
		}
		
		void Label::SetFont(Font *font)
		{
			if(_font)
				_font->Release();
			
			_font = font->Retain();
			_isDirty = true;
			
			DrawMaterial()->RemoveTextures();
			DrawMaterial()->AddTexture(_font->Texture());
		}
		
		void Label::SetText(String *text)
		{
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
				
				TextStyle style(Vector2(Frame().width, Frame().height));
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
