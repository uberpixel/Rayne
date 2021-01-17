//
//  RNUILabel.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UILABEL_H_
#define __RAYNE_UILABEL_H_

#include "RNUIView.h"
#include "RNUIAttributedString.h"

namespace RN
{
	namespace UI
	{
		class Label : public View
		{
		public:
			UIAPI Label(const TextAttributes &defaultAttributes);
			UIAPI ~Label();
			
			UIAPI void SetText(String *text);
			UIAPI void SetAttributedText(AttributedString *text);
			
			UIAPI void SetDefaultAttributes(const TextAttributes &attributes);
			UIAPI void SetTextColor(const Color &color);
			UIAPI void SetVerticalAlignment(TextVerticalAlignment alignment);
			
            UIAPI void SetLineHeight(float lineHeight);
			UIAPI void SetShadowColor(Color color);
			UIAPI void SetShadowOffset(Vector2 offset);
			
			const String *GetText() const { return _attributedText; }

		protected:
			UIAPI virtual void UpdateModel() override;

		private:
			AttributedString *_attributedText;
			TextAttributes _defaultAttributes;
			TextVerticalAlignment _verticalAlignment;
			
            float _lineHeight;
			Color _shadowColor;
			Vector2 _shadowOffset;

			RNDeclareMetaAPI(Label, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILABEL_H_ */
