//
//  RNUICEventManager.cpp
//  Rayne-UIComponents
//
//  Copyright 2024 by twhlynch. All rights reserved.
//

#include "RNUI.h"
#include "RNUICEventManager.h"
#include <vector>
#include <functional>

namespace RN
{
    namespace UIComponents
    {

        EventManager::EventManager()
        {
            
        }

        EventManager::~EventManager()
        {
            
        };

        void EventManager::AddClickEvent(Component* component, std::function<void()> function)
        {
            _clickComponentList.push_back(component);
            _clickEventList.push_back(function);
        }

        void EventManager::ComputeClickEvent(RN::Vector2 click)
        {
            for (size_t i = 0; i < _clickComponentList.size(); i++)
            {
                Component *component = _clickComponentList[i];
                RN::UI::View *view = component->GetView();
                RN::Rect frame = view->GetFrame();
                if (frame.ContainsPoint(click))
                {
                    _clickEventList[i]();
                }
            }
        }
    }
}
