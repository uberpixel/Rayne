//
//  RNUIWidgetInternals.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIWidgetInternals.h"
#include "RNUIStyle.h"
#include "RNLogging.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(WidgetBackgroundView, View)
		RNDefineMeta(WidgetTitleBar, View)
		
		// ---------------------
		// MARK: -
		// MARK: WidgetTitleBar
		// ---------------------
		
		WidgetTitleBar::WidgetTitleBar(Widget *widget, Widget::Style style) :
			_container(widget),
			_style(style),
			_closeButton(nullptr),
			_minimizeButton(nullptr),
			_maximizeButton(nullptr)
		{
			Style *styleSheet = Style::GetSharedInstance();
			
			_title = new Label();
			_title->SetAlignment(TextAlignment::Center);
			_title->SetTextColor(styleSheet->GetColor(Style::ColorStyle::TitleColor));
			_title->SetFont(styleSheet->GetFont(Style::FontStyle::DefaultFontBold));
			
			AddSubview(_title);
		}
		
		WidgetTitleBar::~WidgetTitleBar()
		{
			_title->Release();
			
			SafeRelease(_closeButton);
			SafeRelease(_minimizeButton);
			SafeRelease(_maximizeButton);
		}
		
		void WidgetTitleBar::SetTitle(String *title)
		{
			_title->SetText(title);
			SetNeedsLayoutUpdate();
		}
		
		void WidgetTitleBar::CreateButton(Widget::TitleControl buttonStyle)
		{
			Style *styleSheet = Style::GetSharedInstance();
			Dictionary *style = nullptr;
			bool enabled = false;
			
			try
			{
				switch(buttonStyle)
				{
					case Widget::TitleControl::Close:
						style   = styleSheet->GetButtonStyleWithKeyPath(RNCSTR("window.controls.close"));
						enabled = (_style & Widget::Style::Closable);
						break;
					case Widget::TitleControl::Maximize:
						style   = styleSheet->GetButtonStyleWithKeyPath(RNCSTR("window.controls.maximize"));
						enabled = (_style & Widget::Style::Maximizable);
						break;
					case Widget::TitleControl::Minimize:
						style   = styleSheet->GetButtonStyleWithKeyPath(RNCSTR("window.controls.minimize"));
						enabled = (_style & Widget::Style::Minimizable);
						break;
				}
			}
			catch(Exception e)
			{
				style = nullptr;
			}
			
			if(!style)
				return;
			
			Button *button = new Button(style);
			button->SetImagePosition(ImagePosition::ImageOnly);
			button->SizeToFit();
			button->SetEnabled(enabled);
			
			AddSubview(button);
			_controlButtons.AddObject(button);
			
			switch(buttonStyle)
			{
				case Widget::TitleControl::Close:
					_closeButton = button;
					_closeButton->AddListener(Control::EventType::MouseUpInside, [&](Control *control, Control::EventType event) {
						_container->Close();
					}, nullptr);
					break;
				case Widget::TitleControl::Maximize:
					_maximizeButton = button;
					break;
				case Widget::TitleControl::Minimize:
					_minimizeButton = button;
					break;
			}
		}
		
		void WidgetTitleBar::MouseDragged(Event *event)
		{
			Rect frame = _container->GetFrame();
			frame.x -= event->GetMouseDelta().x;
			frame.y -= event->GetMouseDelta().y;
			
			_container->SetFrame(frame);
		}
		
		void WidgetTitleBar::LayoutSubviews()
		{
			// Size the controls
			Rect bounds = GetBounds();
			float offsetX = 5.0f;
			
			_controlButtons.Enumerate<Button>([&](Button *button, size_t index, bool &stop) {
				
				Rect frame = button->GetFrame();
				
				frame.x = offsetX;
				frame.y = roundf((bounds.height * 0.5) - (frame.height * 0.5f));
				
				offsetX += frame.width;
				button->SetFrame(frame);
				
			});
			
			// Size the title
			{
				float spaceLeft = bounds.width - offsetX - 5.0f;
				Vector2 size   = _title->GetSizeThatFits();
				
				bool fitsCentered = (size.x < spaceLeft);
				float top = roundf((bounds.height * 0.4) - (size.y * 0.5f));
				
				Rect rect = _title->GetFrame();
				
				if(!fitsCentered)
				{
					rect.x = offsetX;
					rect.width = bounds.width - rect.x - 5.0f;
					
					_title->SetAlignment(TextAlignment::Left);
				}
				else
				{
					rect.x = 0.0f;
					rect.width = bounds.width;
					
					_title->SetAlignment(TextAlignment::Center);
				}
				
				rect.y = top;
				rect.height = size.y;
				
				_title->SetFrame(rect);
			}
		}
		
		// ---------------------
		// MARK: -
		// MARK: WidgetBackgroundView
		// ---------------------
		
		WidgetBackgroundView::WidgetBackgroundView(Widget *widget, Widget::Style tstyle, Dictionary *style) :
			_container(widget),
			_style(tstyle)
		{
			Dictionary *selected   = style->GetObjectForKey<Dictionary>(RNCSTR("selected"));
			Dictionary *deselected = style->GetObjectForKey<Dictionary>(RNCSTR("deselected"));
			Dictionary *border     = style->GetObjectForKey<Dictionary>(RNCSTR("border"));
			
			if(selected)
				ParseStyle(selected, Control::State::Selected);
			
			if(deselected)
				ParseStyle(deselected, Control::State::Normal);
			
			if(border)
				_border = Style::ParseEdgeInsets(border);
			
			_backdrop = new ImageView();
			_backdrop->SetAutoresizingMask(AutoresizingMask::FlexibleHeight | AutoresizingMask::FlexibleWidth);
			
			_shadow = new ImageView();
			_shadow->SetAutoresizingMask(AutoresizingMask::FlexibleHeight | AutoresizingMask::FlexibleWidth);
			
			_titleBar = new WidgetTitleBar(widget, tstyle);
			_titleBar->SetAutoresizingMask(AutoresizingMask::FlexibleWidth);
			_titleBar->SetFrame(Rect(0.0f, -_border.top, 0.0f, _border.top));
			
			AddSubview(_shadow);
			AddSubview(_backdrop);
			AddSubview(_titleBar);
			
			if(!(_style & Widget::Style::Titled))
				_titleBar->SetHidden(true);
			
			SetState(Control::State::Selected);
			
			_titleBar->CreateButton(Widget::TitleControl::Close);
			_titleBar->CreateButton(Widget::TitleControl::Minimize);
			_titleBar->CreateButton(Widget::TitleControl::Maximize);
		}
		
		WidgetBackgroundView::~WidgetBackgroundView()
		{
			_backdrop->Release();
			_shadow->Release();
			_titleBar->Release();
		}
		
		void WidgetBackgroundView::LayoutSubviews()
		{
			View::LayoutSubviews();
			
			Rect bounds = GetBounds();
			Rect backdropFrame = bounds;
			
			
			{
				backdropFrame.y      -= _border.top;
				backdropFrame.height += _border.top + _border.bottom;
				
				backdropFrame.x      -= _border.left;
				backdropFrame.width  += _border.left + _border.right;
				
				_backdrop->SetFrame(backdropFrame);
			}
			
			if(_shadowExtents.GetValueForState(_state))
			{
				Rect frame = backdropFrame;
				EdgeInsets insets = Style::ParseEdgeInsets(_shadowExtents.GetValueForState(_state));
				
				frame.y      -= insets.top;
				frame.height += insets.top + insets.bottom;
				
				frame.x      -= insets.left;
				frame.width  += insets.left + insets.right;
				
				_shadow->SetFrame(frame);
			}
		}
		
		void WidgetBackgroundView::SetState(Control::State state)
		{
			_backdrop->SetImage(_backdropImages.GetValueForState(state));
			_shadow->SetImage(_shadowImages.GetValueForState(state));
			
			_state = state;
			SetNeedsLayoutUpdate();
		}
		
		void WidgetBackgroundView::SetTitle(String *title)
		{
			_titleBar->SetTitle(title);
		}
		
		
		void WidgetBackgroundView::ParseStyle(Dictionary *style, Control::State state)
		{
			Style *styleSheet = Style::GetSharedInstance();
			Texture *texture = styleSheet->GetTextureWithName(style->GetObjectForKey<String>(RNCSTR("texture")));
			
			Dictionary *atlas  = style->GetObjectForKey<Dictionary>(RNCSTR("atlas"));
			Dictionary *insets = style->GetObjectForKey<Dictionary>(RNCSTR("insets"));
			Dictionary *shadow = style->GetObjectForKey<Dictionary>(RNCSTR("shadow"));
			
			Image *image = new Image(texture);
			image->SetAtlas(Style::ParseAtlas(atlas), false);
			image->SetEdgeInsets(Style::ParseEdgeInsets(insets));
			
			_backdropImages.SetValueForState(image, state);
			image->Release();
			
			if(shadow)
			{
				atlas  = shadow->GetObjectForKey<Dictionary>(RNCSTR("atlas"));
				insets = shadow->GetObjectForKey<Dictionary>(RNCSTR("insets"));
				
				Image *image = new Image(texture);
				image->SetAtlas(Style::ParseAtlas(atlas), false);
				image->SetEdgeInsets(Style::ParseEdgeInsets(insets));
				
				_shadowImages.SetValueForState(image, state);
				image->Release();
				
				Dictionary *extents = shadow->GetObjectForKey<Dictionary>(RNCSTR("extents"));
				if(extents)
					_shadowExtents.SetValueForState(extents, state);
			}
		}
	}
}
