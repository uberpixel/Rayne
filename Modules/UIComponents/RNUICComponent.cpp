//
//  RNUICComponent.cpp
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUI.h"
#include "RNUICComponent.h"
#include <sstream>

namespace RN
{
    namespace UIComponents
    {
        RNDefineMeta(Component, Object);

        Component::Component(const String *filepath)
            : _view(nullptr), _scroll(nullptr), _label(nullptr), _image(nullptr), _children(new Array())
        {
            Data *data = Data::WithContentsOfFile(filepath);
            Dictionary *dictionary = JSONSerialization::ObjectFromData<Dictionary>(data);
            Dictionary *component = dictionary->GetObjectForKey<Dictionary>(RNCSTR("component"));
            if (component)
            {
                LoadComponent(component);
            }
        }

        Component::Component(Dictionary *dictionary)
            : _view(nullptr), _scroll(nullptr), _label(nullptr), _image(nullptr), _children(new Array())
        {
            Dictionary *component = dictionary->GetObjectForKey<Dictionary>(RNCSTR("component"));
            if (component)
            {
                LoadComponent(component);
            }
            else
            {
                LoadComponent(dictionary);
            }
        }

        Component::~Component()
        {
            SafeRelease(_children);
            SafeRelease(_view);
            SafeRelease(_scroll);
            SafeRelease(_label);
            SafeRelease(_image);
        };

        void Component::LoadComponent(Dictionary *component)
        {
            _component = component;
            
            String *type = component->GetObjectForKey<String>(RNCSTR("type"));
            Dictionary *style = component->GetObjectForKey<Dictionary>(RNCSTR("style"));
            String *text = component->GetObjectForKey<String>(RNCSTR("text"));
            
            float width = GetFloatFromDictionary(style, RNCSTR("width"));
            float height = GetFloatFromDictionary(style, RNCSTR("height"));
            float top = GetFloatFromDictionary(style, RNCSTR("top"));
            float left = GetFloatFromDictionary(style, RNCSTR("left"));
            float fontSize = GetFloatFromDictionary(style, RNCSTR("fontSize"));

            String *color = GetStringFromDictionary(style, RNCSTR("color"));
            String *background = GetStringFromDictionary(style, RNCSTR("backgroundColor"));
            String *highlightColor = GetStringFromDictionary(style, RNCSTR("highlightColor"));
            String *highlightBackground = GetStringFromDictionary(style, RNCSTR("highlightBackgroundColor"));
            String *textAlign = GetStringFromDictionary(style, RNCSTR("textAlign"));
            String *textVerticalAlign = GetStringFromDictionary(style, RNCSTR("textVerticalAlign"));
            String *shadow = GetStringFromDictionary(style, RNCSTR("textShadow"));

            Color textColor = color ? ColorFromHexString(color) : Color::White();
            Color backgroundColor = background ? ColorFromHexString(background) : Color::WithRGBA(0, 0, 0, 0);
            Color highlightTextColor = highlightColor ? ColorFromHexString(highlightColor) : Color();
            Color highlightBackgroundColor = highlightBackground ? ColorFromHexString(highlightBackground) : Color();
            Color shadowColor = NULL;
            Vector2 shadowOffset(0.0f);
            
            if (shadow)
            {
                Array *shadowComponents = shadow->GetComponentsSeparatedByString(RNCSTR(" "));
                const String *offsetX = shadowComponents->GetObjectAtIndex(0)->GetDescription();
                const String *offsetY = shadowComponents->GetObjectAtIndex(1)->GetDescription();
                shadowOffset.x = std::stoi(offsetX->GetUTF8String());
                shadowOffset.y = std::stoi(offsetY->GetUTF8String());
                String *shadowColorComponent = new String(shadowComponents->GetObjectAtIndex(2)->GetDescription());
                shadowColor = ColorFromHexString(shadowColorComponent);
                shadowColorComponent->Release();
            }

            UI::TextAlignment textAlignment = UI::TextAlignmentLeft;
            UI::TextVerticalAlignment textVerticalAlignment = UI::TextVerticalAlignmentTop;
            if (textAlign)
            {
                if (textAlign->IsEqual(RNCSTR("right")))
                {
                    textAlignment = UI::TextAlignmentRight;
                }
                else if (textAlign->IsEqual(RNCSTR("center")))
                {
                    textAlignment = UI::TextAlignmentCenter;
                }
            }
            if (textVerticalAlign)
            {
                if (textVerticalAlign->IsEqual(RNCSTR("bottom")))
                {
                    textVerticalAlignment = UI::TextVerticalAlignmentBottom;
                }
                else if (textVerticalAlign->IsEqual(RNCSTR("center")))
                {
                    textVerticalAlignment = UI::TextVerticalAlignmentCenter;
                }
            }
            
            UI::FontManager *fontManager = UI::FontManager::GetSharedInstance();
            UI::Font *font = fontManager->GetFontForFilepath(RNCSTR("fonts/Arial.ttf"));
            UI::TextAttributes textAttr = UI::TextAttributes(font, fontSize, textColor, textAlignment);
            
            Rect frame = Rect(left, top, width, height);
            if (type)
            {
                if (type->IsEqual(RNCSTR("button")))
                {
                    UI::Button *button = new UI::Button(textAttr);
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
                    UI::Label *label = new UI::Label(textAttr);
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
                    String *src = component->GetObjectForKey<String>(RNCSTR("src"));
                    UI::ImageView *image = new UI::ImageView();
                    
                    if (src)
                    {
                        Texture *imageTexture = Texture::WithName(src);
                        image = new UI::ImageView(imageTexture);
                    }
                    
                    image->SetFrame(frame);
                    
                    _image = image;
                    _view = image;
                }
                else if (type->IsEqual(RNCSTR("slider")))
                {
                    float minimum = GetFloatFromDictionary(component, RNCSTR("min"));
                    float maximum = GetFloatFromDictionary(component, RNCSTR("max"));
                    float value = GetFloatFromDictionary(component, RNCSTR("value"));
                    float step = GetFloatFromDictionary(component, RNCSTR("step"));
                    
                    UI::Slider *slider = new UI::Slider(frame, value, minimum, maximum, step);
                    _view = slider;
                }
                else if (type->IsEqual(RNCSTR("view")))
                {
                    UI::View *view = new UI::View();
                    view->SetFrame(frame);
                    view->SetBackgroundColor(backgroundColor);
                    
                    _view = view;
                }
                else if (type->IsEqual(RNCSTR("scroll")))
                {
                    bool vertical = GetBooleanFromDictionary(component, RNCSTR("vertical"));
                    bool horizontal = GetBooleanFromDictionary(component, RNCSTR("horizontal"));

                    UI::ScrollView *view = new UI::ScrollView(vertical, horizontal);
                    view->SetFrame(frame);
                    view->SetBackgroundColor(backgroundColor);
                    
                    _view = view;
                    _scroll = view;
                }
                
                
                Array *children = component->GetObjectForKey<Array>(RNCSTR("children"));
                if(children)
                {
                    children->Enumerate<Dictionary>([&](Dictionary *child, size_t index, bool &stop){
                        Dictionary *childComponent = child->GetObjectForKey<Dictionary>(RNCSTR("component"));
                        Component *component = new Component(childComponent);
                        AddChild(component->Autorelease());
                    });
                }
            }
        }
        
        void Component::AddChild(UI::View *view)
        {
            if (!_view) return;
            _view->AddSubview(view);
        }

        void Component::AddChild(Component *component)
        {
            if (!_view) return;
            _children->AddObject(component);
            UI::View *view = component->GetView();
            _view->AddSubview(view->Autorelease());
        }

        void Component::SetText(const String *text)
        {
            if (!_label) return;
            _label->SetText(text);
        }

        void Component::SetPosition(int x, int y)
        {
            Dictionary *style = _component->GetObjectForKey<Dictionary>(RNCSTR("style"));
            Number *width = style->GetObjectForKey<Number>(RNCSTR("width"));
            Number *height = style->GetObjectForKey<Number>(RNCSTR("height"));
            
            _view->SetFrame(Rect(x, y, width->GetFloatValue(), height->GetFloatValue()));
        }

        UI::View* Component::GetView()
        {
            return _view;
        }

        Component* Component::GetComponent(int index) {
            return _children->GetObjectAtIndex<Component>(index);
        }

        UI::ScrollView* Component::GetScrollView()
        {
            return _scroll;
        }

        void Component::SetImage(RN::Texture *texture)
        {
            if(!_image) return;
            _image->SetImage(texture);
        }

        Color Component::ColorFromHexString(String *hexString)
        {
            uint32 colorCode = 0x00000000;
            Size length = hexString->GetLength();
            if (hexString->HasPrefix(RNCSTR("#")))
            {
                hexString = hexString->GetSubstring(Range(1, length));
            }
            if (hexString->GetLength() == 6)
            {
                hexString->Append("ff");
            }
            
            std::stringstream ss;
            ss << std::hex << hexString->GetUTF8String();
            ss >> colorCode;
            
            return Color::WithHex(colorCode);
        }

        float Component::GetFloatFromDictionary(Dictionary *dict, const String *key)
        {
            Number *number = dict->GetObjectForKey<Number>(key);
            float value = number ? number->GetFloatValue() : 0.0f;
            return value;
        }
    
        bool Component::GetBooleanFromDictionary(Dictionary *dict, const String *key)
        {
            Number *number = dict->GetObjectForKey<Number>(key);
            bool value = number ? number->GetBoolValue() : false;
            return value;
        }

        String* Component::GetStringFromDictionary(Dictionary *dict, const String *key)
        {
            String *str = dict->GetObjectForKey<String>(key);
            if (str) str->Retain();
            return str;
        }
    }
}
