//
//  RNUICEventManager.h
//  Rayne-UIComponents
//
//  Copyright 2024 by twhlynch. All rights reserved.
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
            void ComputeClickEvent(RN::Vector2 click);

        private:
            std::vector<Component*> _clickComponentList;
            std::vector<std::function<void()>> _clickEventList;
        };
    }
}


#endif /* __RNUICOMPONENTS_EVENTMANAGER_H_ */
