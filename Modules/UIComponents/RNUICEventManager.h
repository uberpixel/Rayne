//
//  RNUICEventManager.h
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RNUICOMPONENTS_EVENTMANAGER_H_
#define __RNUICOMPONENTS_EVENTMANAGER_H_

#include <Rayne.h>
#include "RNUI.h"
#include "RNUICComponent.h"
#include <vector>

namespace RN
{
    namespace UIComponents
    {
        class EventManager
        {
        public:
            EventManager();
            ~EventManager();
            
            void AddClickEvent(Component* component, std::function<void()> function);
            void RemoveClickEvent(Component* component);
            void ComputeClickEvent(Vector2 click);

        private:
            std::vector<Component*> _clickComponentList;
            std::vector<std::function<void()>> _clickEventList;
        };
    }
}


#endif /* __RNUICOMPONENTS_EVENTMANAGER_H_ */
