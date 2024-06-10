//
//  RNUICComponent.h
//  Rayne
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RNUICOMPONENTS_COMPONENT_H_
#define __RNUICOMPONENTS_COMPONENT_H_

#include <Rayne.h>
#include "RNUI.h"

namespace RN
{
    namespace UIComponents
    {
        class Component : public Object
        {
        public:
            UIAPI Component(const String *filepath);
            UIAPI Component(Dictionary *dictionary);
            UIAPI ~Component();
            
            UIAPI void AddChild(UI::View *view);
            UIAPI void AddChild(Component *component);
            
            UIAPI void SetText(const String *text);
            UIAPI void SetPosition(int x, int y);
            
            UI::View* GetView();
            Component* GetComponent(int index);
            UI::ScrollView* GetScrollView();
            
            UIAPI void SetImage(Texture *texture);

        private:
            UI::View *_view;
            UI::ScrollView *_scroll;
            UI::Label *_label;
            UI::ImageView *_image;
            
            Array *_children;
            Dictionary *_component;
            
            void LoadComponent(Dictionary *component);
            Color ColorFromHexString(String *hexString);
            float GetFloatFromDictionary(Dictionary *dict, const String *key);
            String* GetStringFromDictionary(Dictionary *dict, const String *key);
            
            RNDeclareMetaAPI(Component, UIAPI)
        };
    }
}

#endif /* __RNUICOMPONENTS_COMPONENT_H_ */
