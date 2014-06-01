//
//  RNUIColorView.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIColorView.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ColorView, Control)
		
		ColorView::ColorView() :
			_colorPicker(nullptr)
		{
			Initialize(Style::GetSharedInstance()->GetResourceWithKeyPath<Dictionary>(RNCSTR("colorPicker")));
			SetColor(RN::Color::White());
		}
		
		ColorView::ColorView(Dictionary *style) :
			_colorPicker(nullptr)
		{
			Initialize(style);
			SetColor(RN::Color::White());
		}
		
		ColorView::~ColorView()
		{
			if(_colorPicker)
			{
				_colorPicker->Close();
				_colorPicker->Release();
			}
			
			_border->Release();
			_contentView->Release();
		}
		
		void ColorView::Initialize(Dictionary *style)
		{
			_border = new ImageView();
			_contentView = new View();
			
			AddSubview(_border);
			AddSubview(_contentView);
			
			if(!style)
				return;
			
			Style *styleSheet = Style::GetSharedInstance();
			
			Dictionary *border = style->GetObjectForKey<Dictionary>(RNCSTR("border"));
			Dictionary *insets = style->GetObjectForKey<Dictionary>(RNCSTR("contentInsets"));
			
			if(insets)
				_insets = Style::ParseEdgeInsets(insets);
			
			if(border)
			{
				Texture *texture = styleSheet->GetTextureWithName(border->GetObjectForKey<String>(RNCSTR("texture")));
				Array *states    = border->GetObjectForKey<Array>(RNCSTR("states"));
				
				states->Enumerate<Dictionary>([&](Dictionary *state, size_t index, bool &stop) {
					String *name = state->GetObjectForKey<String>(RNCSTR("name"));
					Dictionary *atlas  = state->GetObjectForKey<Dictionary>(RNCSTR("atlas"));
					Dictionary *insets = state->GetObjectForKey<Dictionary>(RNCSTR("insets"));
					
					State tstate = Style::ParseState(name);
					Image *image = new Image(texture);
					
					if(atlas)
						image->SetAtlas(Style::ParseAtlas(atlas), false);
					
					if(insets)
						image->SetEdgeInsets(Style::ParseEdgeInsets(insets));
					
					_borderImages.SetValueForState(image->Autorelease(), tstate);
				});
			}
			
			StateChanged(GetState());
		}
		
		
		
		void ColorView::SetColor(const RN::Color &color)
		{
			_color = color;
			_contentView->SetBackgroundColor(Color::WithRNColor(color)->GetRNColor());
			
			if(_colorPicker)
			{
				ColorPicker *picker = _colorPicker->GetContentView()->GetSubivews()->GetObjectAtIndex<ColorPicker>(0);
				picker->SetColor(_color);
			}
		}
		
		void ColorView::SetColorInternal(const RN::Color &color)
		{
			if(_color == color)
				return;
			
			_color = color;
			_contentView->SetBackgroundColor(Color::WithRNColor(color)->GetRNColor());
			
			DispatchEvent(EventType::ValueChanged);
		}
		
		void ColorView::StateChanged(State state)
		{
			Control::StateChanged(state);
			
			Image *image = _borderImages.GetValueForState(state);
			_border->SetImage(image);
			
			SetNeedsLayoutUpdate();
		}
		
		void ColorView::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect frame = GetBounds();
			
			_border->SetFrame(frame);
			_contentView->SetFrame(Rect(_insets.left, _insets.top, frame.width - (_insets.left + _insets.right), frame.height - (_insets.top + _insets.bottom)));
		}
		
		bool ColorView::PostEvent(EventType event)
		{
			if(event == EventType::MouseUpInside)
			{
				if(!_colorPicker)
				{
					ColorPicker *picker = new ColorPicker();
					picker->SetFrame(Rect(0.0, 0.0, 200.0f, 180.0f).Inset(5.0f, 5.0f));
					picker->SetColor(_color);
					picker->AddListener(EventType::ValueChanged, [this](Control *control, EventType event) {
						
						ColorPicker *picker = control->Downcast<ColorPicker>();
						SetColorInternal(picker->GetColor());
						
					}, nullptr);
					
					_colorPicker = new Widget(Widget::Style::Titled | Widget::Style::Closable);
					_colorPicker->SetTitle(RNCSTR("Color Picker"));
					_colorPicker->SetFrame(Rect(100, 100, 200.0f, 180.0f));
					_colorPicker->GetContentView()->AddSubview(picker);
				}
				
				_colorPicker->Open();
			}
			
			return Control::PostEvent(event);
		}
	}
}