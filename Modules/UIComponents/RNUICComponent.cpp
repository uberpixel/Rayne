//
//  RNUICComponent.cpp
//  Rayne-UIComponents
//
//  Copyright 2024 by twhlynch. All rights reserved.
//

#include "RNUI.h"
#include "RNUICComponent.h"
#include <sstream>

namespace RN
{
    namespace UIComponents
    {
        RNDefineMeta(Component, RN::Object);

        Component::Component(const RN::String *filepath)
        {
            _view = nullptr;
            _scroll = nullptr;
            _label = nullptr;
            _image = nullptr;
            _children = new RN::Array();
            RN::Data *data = RN::Data::WithContentsOfFile(filepath);
            RN::Dictionary *dictionary = RN::JSONSerialization::ObjectFromData<RN::Dictionary>(data);
            RN::Dictionary *component = dictionary->GetObjectForKey<RN::Dictionary>(RNCSTR("component"));
            if (component)
            {
                LoadComponent(component);
            }
        }

        Component::Component(RN::Dictionary *component)
        {
            _view = nullptr;
            _scroll = nullptr;
            _label = nullptr;
            _image = nullptr;
            _children = new RN::Array();
            if (component)
            {
                LoadComponent(component);
            }
        }

        Component::~Component()
        {
            
        };

        void Component::LoadComponent(RN::Dictionary *component)
        {
            _component = component;
            
            RN::String *type = component->GetObjectForKey<RN::String>(RNCSTR("type"));
            RN::String *id = component->GetObjectForKey<RN::String>(RNCSTR("id"));
            RN::Dictionary *style = component->GetObjectForKey<RN::Dictionary>(RNCSTR("style"));
            RN::String *text = component->GetObjectForKey<RN::String>(RNCSTR("text"));
            
            float width = NULL;
            float height = NULL;
            float top = NULL;
            float left = NULL;
            float fontSize = NULL;
            RN::String *color = nullptr;
            RN::String *background = nullptr;
            RN::String *highlightColor = nullptr;
            RN::String *highlightBackground = nullptr;
            RN::String *textAlign = nullptr;
            RN::String *shadow = nullptr;
            
            if (style)
            {
                RN::Number *widthValue = style->GetObjectForKey<RN::Number>(RNCSTR("width"));
                if (widthValue) {
                    width = widthValue->GetFloatValue();
                }
                RN::Number *heightValue = style->GetObjectForKey<RN::Number>(RNCSTR("height"));
                if (heightValue) {
                    height = heightValue->GetFloatValue();
                }
                RN::Number *topValue = style->GetObjectForKey<RN::Number>(RNCSTR("top"));
                if (topValue) {
                    top = topValue->GetFloatValue();
                }
                RN::Number *leftValue = style->GetObjectForKey<RN::Number>(RNCSTR("left"));
                if (leftValue) {
                    left = leftValue->GetFloatValue();
                }
                RN::Number *fontSizeValue = style->GetObjectForKey<RN::Number>(RNCSTR("fontSize"));
                if (fontSizeValue) {
                    fontSize = fontSizeValue->GetFloatValue();
                }
                color = style->GetObjectForKey<RN::String>(RNCSTR("color"));
                background = style->GetObjectForKey<RN::String>(RNCSTR("backgroundColor"));
                textAlign = style->GetObjectForKey<RN::String>(RNCSTR("textAlign"));
                shadow = style->GetObjectForKey<RN::String>(RNCSTR("textShadow"));
                
                highlightColor = style->GetObjectForKey<RN::String>(RNCSTR("highlightColor"));
                highlightBackground = style->GetObjectForKey<RN::String>(RNCSTR("highlightBackgroundColor"));
            }

            RN::Color textColor = RN::Color::White();
            RN::Color backgroundColor = RN::Color::WithRGBA(0, 0, 0, 0);
            RN::Color highlightTextColor = NULL;
            RN::Color highlightBackgroundColor = NULL;
            RN::Color shadowColor = NULL;
            RN::Vector2 shadowOffset(0.0f);
            
            if (color)
            {
                textColor = ColorFromHexString(color);
            }
            if (background)
            {
                backgroundColor = ColorFromHexString(background);
            }
            if (highlightColor)
            {
                highlightTextColor = ColorFromHexString(highlightColor);
            }
            if (highlightBackground)
            {
                highlightBackgroundColor = ColorFromHexString(highlightBackground);
            }
            if (shadow)
            {
                RN::Array *shadowComponents = shadow->GetComponentsSeparatedByString(RNCSTR(" "));
                RN::String *offsetX = new RN::String(shadowComponents->GetObjectAtIndex(0)->GetDescription());
                RN::String *offsetY = new RN::String(shadowComponents->GetObjectAtIndex(1)->GetDescription());
                shadowOffset.x = std::stoi(offsetX->GetUTF8String());
                shadowOffset.y = std::stoi(offsetY->GetUTF8String());
                offsetX->Release();
                offsetY->Release();
                RN::String *shadowColorComponent = new RN::String(shadowComponents->GetObjectAtIndex(2)->GetDescription());
                shadowColor = ColorFromHexString(shadowColorComponent);
                shadowColorComponent->Release();
            }

            RN::UI::TextAlignment textAlignment = RN::UI::TextAlignmentLeft;
            RN::UI::TextVerticalAlignment textVerticalAlignment = RN::UI::TextVerticalAlignmentTop;
            if (textAlign)
            {
                RN::Array *alignComponents = textAlign->GetComponentsSeparatedByString(RNCSTR(" "));
                if (alignComponents->GetCount() == 1) {
                    alignComponents->AddObject(alignComponents->GetObjectAtIndex(0));
                }
                RN::String *alignX = new RN::String(alignComponents->GetObjectAtIndex(0)->GetDescription());
                RN::String *alignY = new RN::String(alignComponents->GetObjectAtIndex(1)->GetDescription());
                if (alignX->IsEqual(RNCSTR("right")))
                {
                    textAlignment = RN::UI::TextAlignmentRight;
                }
                else if (alignX->IsEqual(RNCSTR("center")))
                {
                    textAlignment = RN::UI::TextAlignmentCenter;
                }

                if (alignY->IsEqual(RNCSTR("bottom")))
                {
                    textVerticalAlignment = RN::UI::TextVerticalAlignmentBottom;
                }
                else if (alignY->IsEqual(RNCSTR("center")))
                {
                    textVerticalAlignment = RN::UI::TextVerticalAlignmentCenter;
                }
                alignX->Release();
                alignY->Release();
            }
            
            RN::UI::FontManager *fontManager = RN::UI::FontManager::GetSharedInstance();
            RN::UI::Font *font = fontManager->GetFontForFilepath(RNCSTR("fonts/Arial.ttf"));
            RN::UI::TextAttributes textAttr = RN::UI::TextAttributes(font, fontSize, textColor, textAlignment);
            
            RN::Rect frame = RN::Rect(left, top, width, height);
            if (type)
            {
                if (type->IsEqual(RNCSTR("button")))
                {
                    RN::UI::Button *button = new RN::UI::Button(textAttr);
                    button->GetLabel()->SetText(text);
                    button->SetFrame(frame);
                    button->SetBackgroundColorNormal(backgroundColor);
                    button->GetLabel()->SetVerticalAlignment(textVerticalAlignment);
                    
                    if (highlightColor)
                    {
                        button->SetTextColorNormal(textColor);
                        button->SetTextColorHighlight(highlightTextColor);
                    }
                    if (highlightBackground)
                    {
                        button->SetBackgroundColorHighlight(highlightBackgroundColor);
                    }
                    if (shadow)
                    {
                        button->GetLabel()->SetShadowOffset(shadowOffset);
                        button->GetLabel()->SetShadowColor(shadowColor);
                    }

                    _view = button;
                    _label = button->GetLabel();
                }
                else if (type->IsEqual(RNCSTR("label")))
                {
                    RN::UI::Label *label = new RN::UI::Label(textAttr);
                    label->SetText(text);
                    label->SetFrame(frame);
                    label->SetBackgroundColor(backgroundColor);
                    label->SetVerticalAlignment(textVerticalAlignment);
                    
                    if (shadow)
                    {
                        label->SetShadowOffset(shadowOffset);
                        label->SetShadowColor(shadowColor);
                    }

                    _view = label;
                    _label = label;
                }
                else if (type->IsEqual(RNCSTR("image")))
                {
                    RN::String *src = component->GetObjectForKey<RN::String>(RNCSTR("src"));
                    RN::UI::ImageView *image = new RN::UI::ImageView();
                    
                    if (src)
                    {
                        RN::Texture *imageTexture = RN::Texture::WithName(src);
                        image = new RN::UI::ImageView(imageTexture);
                    }
                    
                    image->SetFrame(frame);
                    
                    _image = image;
                    _view = image;
                }
                else if (type->IsEqual(RNCSTR("slider")))
                {
                    float minimum, maximum, value, step = NULL;
                    RN::Number *minimumVlaue = component->GetObjectForKey<RN::Number>(RNCSTR("min"));
                    RN::Number *maximumlaue = component->GetObjectForKey<RN::Number>(RNCSTR("max"));
                    RN::Number *valueVlaue = component->GetObjectForKey<RN::Number>(RNCSTR("value"));
                    RN::Number *stepVlaue = component->GetObjectForKey<RN::Number>(RNCSTR("step"));
                    if (minimumVlaue) {
                        minimum = minimumVlaue->GetFloatValue();
                    }
                    if (maximumlaue) {
                        maximum = maximumlaue->GetFloatValue();
                    }
                    if (valueVlaue) {
                        value = valueVlaue->GetFloatValue();
                    }
                    if (stepVlaue) {
                        step = stepVlaue->GetFloatValue();
                    }
                    
                    RN::UI::Slider *slider = new RN::UI::Slider(frame, value, minimum, maximum, step);
                    _view = slider;
                }
                else if (type->IsEqual(RNCSTR("view")))
                {
                    RN::UI::View *view = new RN::UI::View();
                    view->SetFrame(frame);
                    view->SetBackgroundColor(backgroundColor);
                    
                    _view = view;
                    
                    RN::Array *children = component->GetObjectForKey<RN::Array>(RNCSTR("children"));
                    if(children)
                    {
                        children->Enumerate<RN::Dictionary>([&](RN::Dictionary *child, size_t index, bool &stop){
                            RN::Dictionary *childComponent = child->GetObjectForKey<RN::Dictionary>(RNCSTR("component"));
                            AddChild(new Component(childComponent));
                        });
                    }
                }
                else if (type->IsEqual(RNCSTR("scroll")))
                {
                    RN::UI::ScrollView *view = new RN::UI::ScrollView();
                    view->SetFrame(frame);
                    view->SetBackgroundColor(backgroundColor);
                    
                    _view = view;
                    _scroll = view;
                    
                    RN::Array *children = component->GetObjectForKey<RN::Array>(RNCSTR("children"));
                    if(children)
                    {
                        children->Enumerate<RN::Dictionary>([&](RN::Dictionary *child, size_t index, bool &stop){
                            RN::Dictionary *childComponent = child->GetObjectForKey<RN::Dictionary>(RNCSTR("component"));
                            AddChild(new Component(childComponent));
                        });
                    }
                }
            }
        }
        
        void Component::AddChild(RN::UI::View *view)
        {
            if (_view)
            {
                _view->AddSubview(view->Autorelease());
            }
        }

        void Component::AddChild(Component *component)
        {
            if (_view)
            {
                _children->AddObject(component);
                
                RN::UI::View *view = component->GetView();
                _view->AddSubview(view->Autorelease());
            }
        }

        void Component::SetText(const RN::String *text)
        {
            if (_label)
            {
                _label->SetText(text);
            }
        }

        void Component::SetPosition(int x, int y)
        {
            RN::Dictionary *style = _component->GetObjectForKey<RN::Dictionary>(RNCSTR("style"));
            RN::Number *width = style->GetObjectForKey<RN::Number>(RNCSTR("width"));
            RN::Number *height = style->GetObjectForKey<RN::Number>(RNCSTR("height"));
            
            _view->SetFrame(RN::Rect(x, y, width->GetFloatValue(), height->GetFloatValue()));
        }

        RN::UI::View* Component::GetView()
        {
            if (_view)
            {
                return _view;
            }
        }

        Component* Component::GetComponent(int index) {
            return _children->GetObjectAtIndex<Component>(index);
        }

        RN::UI::ScrollView* Component::GetScrollView()
        {
            if (_scroll)
            {
                return _scroll;
            }
        }

        void Component::SetImage(RN::Texture *texture)
        {
            RN::UI::ImageView *image = new RN::UI::ImageView(texture);
            _image = image;
        }
                    
        RN::Color Component::ColorFromHexString(RN::String *hexString)
        {
            RN::uint32 colorCode = 0x00000000;
            Size length = hexString->GetLength();
            if (hexString->HasPrefix(RNCSTR("#")))
            {
                hexString = hexString->GetSubstring(RN::Range(1, length));
            }
            if (hexString->GetLength() == 6)
            {
                hexString->Append("ff");
            }
            
            std::stringstream ss;
            ss << std::hex << hexString->GetUTF8String();
            ss >> colorCode;
            
            return RN::Color::WithHex(colorCode);
        }
    }
}