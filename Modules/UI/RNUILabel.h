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
			UIAPI Vector2 GetCharacterPosition(size_t charIndex);
			UIAPI size_t GetCharacterAtPosition(const Vector2 &position);
			UIAPI Vector2 GetTextSize();
			
			UIAPI void SetCursor(bool enabled, size_t position);
			UIAPI void SetCursor(bool enabled, Vector2 position);
			size_t GetCursorPosition() const { return _currentCursorPosition; }
			bool GetHasCursor() const { return _cursorView != nullptr; }
			
			UIAPI void Update(float delta) override;

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
			Material *_textMaterial;
			Material *_shadowMaterial;
			
			View *_cursorView;
			float _cursorBlinkTimer;
			size_t _currentCursorPosition;

			RNDeclareMetaAPI(Label, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILABEL_H_ */
