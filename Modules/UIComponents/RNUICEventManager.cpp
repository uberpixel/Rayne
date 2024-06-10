//
//  RNUICEventManager.cpp
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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

        void EventManager::RemoveClickEvent(Component* component)
        {
            for (size_t i = 0; i < _clickComponentList.size(); i++)
            {
                if (_clickComponentList[i] == component)
                {
                    _clickComponentList.erase(_clickComponentList.begin() + i);
                    _clickEventList.erase(_clickEventList.begin() + i);
                    return;
                }
            }
        }

        void EventManager::ComputeClickEvent(Vector2 click)
        {
            for (size_t i = 0; i < _clickComponentList.size(); i++)
            {
                Component *component = _clickComponentList[i];
                UI::View *view = component->GetView();
                Rect frame = view->GetFrame();
                if (frame.ContainsPoint(click))
                {
                    _clickEventList[i]();
                }
            }
        }
    }
}
