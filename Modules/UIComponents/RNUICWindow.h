//
//  RNUICWindow.h
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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
        class Window : public UI::Window
        {
        public:
            UIAPI Window(const Rect &frame);
            UIAPI ~Window();
            
            UIAPI void AddComponent(Component *component);
            UIAPI void AddComponent(Dictionary *dictionary);
            UIAPI void AddComponent(String *filepath);
            
            Component* GetComponent(int index);

        private:
            Array *_components;

            RNDeclareMetaAPI(Window, UIAPI);
        };
    }
}


#endif /* __RNUICOMPONENTS_WINDOW_H_ */
