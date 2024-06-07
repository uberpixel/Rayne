//
//  RNUICComponent.h
//  Rayne-UIComponents
//
//  Copyright 2024 by twhlynch. All rights reserved.
//

#ifndef __RNUICOMPONENTS_COMPONENT_H_
#define __RNUICOMPONENTS_COMPONENT_H_

#include <Rayne.h>
#include "RNUI.h"

namespace RN
{
    namespace UIComponents
    {
        class Component : public RN::Object
        {
        public:
            UIAPI Component(const RN::String *filepath);
            UIAPI Component(RN::Dictionary *component);
            UIAPI ~Component();
            
            UIAPI void AddChild(RN::UI::View *view);
            UIAPI void AddChild(Component *component);
            
            UIAPI void SetText(const RN::String *text);
            UIAPI void SetPosition(int x, int y);
            
            RN::UI::View* GetView();
            Component* GetComponent(int index);
            RN::UI::ScrollView* GetScrollView();
            
            UIAPI void SetImage(RN::Texture *texture);

        private:
            RN::UI::View *_view;
            RN::UI::ScrollView *_scroll;
            RN::UI::Label *_label;
            RN::UI::View *_image;
            
            RN::Array *_children;
            RN::Dictionary *_component;
            
            void LoadComponent(RN::Dictionary *component);
            RN::Color ColorFromHexString(RN::String *hexString);
            float GetFloatFromDictionary(RN::Dictionary *dict, const RN::String *key);
            RN::String* GetStringFromDictionary(RN::Dictionary *dict, const RN::String *key);
            
            RNDeclareMetaAPI(Component, UIAPI)
        };
    }
}

#endif /* __RNUICOMPONENTS_COMPONENT_H_ */
