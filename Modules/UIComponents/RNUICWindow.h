//
//  RNUICWindow.h
//  Rayne-UIComponents
//
//  Copyright 2024 by twhlynch. All rights reserved.
//

#ifndef __RNUICOMPONENTS_WINDOW_H_
#define __RNUICOMPONENTS_WINDOW_H_

#include <Rayne.h>
#include "RNUI.h"
#include "RNUICComponent.h"

namespace RN
{
    namespace UIComponents
    {
        class Window : public RN::UI::Window
        {
        public:
            UIAPI Window(const RN::Rect &frame);
            UIAPI ~Window();
            
            UIAPI void AddComponent(Component *component);
            UIAPI void AddComponent(RN::Dictionary *dictionary);
            UIAPI void AddComponent(RN::String *filepath);
            
            RN::UI::View* GetComponent(const RN::String &id);

        private:
            RN::Array *_components;
            RN::Array *_componentIds;

            RNDeclareMetaAPI(Window, UIAPI);
        };
    }
}


#endif /* __RNUICOMPONENTS_WINDOW_H_ */
