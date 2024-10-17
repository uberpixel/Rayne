//
//  RNUICWindow.cpp
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUI.h"
#include "RNUICWindow.h"
#include "RNUICComponent.h"
#include <sstream>

namespace RN
{
    namespace UIComponents
    {
        RNDefineMeta(Window, UI::Window);

        Window::Window(const Rect &frame) : UI::Window(frame)
            , _components(new Array())
        {
            SetFrame(frame);
        }
        Window::~Window()
        {
            _components->Release();
        };

        void Window::AddComponent(Component *component)
        {
            _components->AddObject(component);
            AddSubview(component->GetView()->Autorelease());
        }

        void Window::AddComponent(Dictionary *dictionary)
        {
            Component *component = new Component(dictionary);
            _components->AddObject(component);
            AddSubview(component->GetView()->Autorelease());
            component->Release();
        }

        void Window::AddComponent(String *filepath)
        {
            Component *component = new Component(filepath);
            _components->AddObject(component);
            AddSubview(component->GetView()->Autorelease());
            component->Release();
        }

        Component* Window::GetComponent(const int index)
        {
            if (index < 0 || index >= _components->GetCount()) return nullptr;
            Component *component = _components->GetObjectAtIndex<Component>(index);
            return component;
        }
    }
}
