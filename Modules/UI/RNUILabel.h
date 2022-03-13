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
			
			UIAPI void SetText(const String *text);
			UIAPI void SetAttributedText(AttributedString *text);
			
			UIAPI void SetDefaultAttributes(const TextAttributes &attributes);
			UIAPI void SetTextColor(const Color &color);
			UIAPI void SetVerticalAlignment(TextVerticalAlignment alignment);

			UIAPI void SetAdditionalLineHeight(float lineHeight);
			UIAPI void SetShadowColor(Color color);
			UIAPI void SetShadowOffset(Vector2 offset);
			
			UIAPI void SetTextDepthMode(DepthMode depthMode);
			
			UIAPI void SetTextMaterial(Material *material);
			UIAPI void SetTextShadowMaterial(Material *material);
			
			const String *GetText() const { return _attributedText; }
			UIAPI Vector2 GetTextSize();

		protected:
			UIAPI virtual void UpdateModel() override;

		private:
			AttributedString *_attributedText;
			TextAttributes _defaultAttributes;
			TextVerticalAlignment _verticalAlignment;
			
			float _additionalLineHeight;
			Color _shadowColor;
			Vector2 _shadowOffset;
			
			DepthMode _labelDepthMode;
			RN::Material *_textMaterial;
			RN::Material *_shadowMaterial;

			RNDeclareMetaAPI(Label, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILABEL_H_ */
