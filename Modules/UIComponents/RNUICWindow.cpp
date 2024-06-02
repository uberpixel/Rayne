//
//  RNUICWindow.cpp
//  Rayne-UIComponents
//
//  Copyright 2024 by twhlynch. All rights reserved.
//

#include "RNUI.h"
#include "RNUICWindow.h"
#include "RNUICComponent.h"
#include <sstream>

namespace RN
{
    namespace UIComponents
    {
        RNDefineMeta(Window, RN::UI::Window);

        Window::Window(const RN::Rect &frame) : RN::UI::Window(frame)
        {
            SetFrame(frame);
            _components = new RN::Array();
            _componentIds = new RN::Array();
        }
        Window::~Window()
        {
            _components->Release();
            _componentIds->Release();
        };

        void Window::AddComponent(Component *component)
        {
            AddSubview(component->GetView()->Autorelease());
        }

        void Window::AddComponent(RN::Dictionary *dictionary)
        {
            AddSubview((new Component(dictionary))->GetView()->Autorelease());
        }

        void Window::AddComponent(RN::String *filepath)
        {
            AddSubview((new Component(filepath))->GetView()->Autorelease());
        }

        RN::UI::View* Window::GetComponent(const RN::String &id)
        {
            for (size_t i = 0; i < _components->GetCount(); i++)
            {
                if (id.IsEqual(_componentIds->GetObjectAtIndex<RN::String>(i)))
                {
                    RN::UI::View *component = _components->GetObjectAtIndex<RN::UI::View>(i);
                    return component;
                }
            }

            return nullptr;
        }
    }
}
