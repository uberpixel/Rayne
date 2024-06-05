//
//  RNUIButton.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIBUTTON_H_
#define __RAYNE_UIBUTTON_H_

#include "RNUIView.h"
#include "RNUILabel.h"
#include "RNUIImageView.h"

namespace RN
{
	namespace UI
	{
		class Button : public ImageView
		{
		public:
			UIAPI Button(const TextAttributes &defaultTextAttributes);
			UIAPI ~Button();
			
			UIAPI void SetFrame(const Rect &frame) override;

			UIAPI void SetImageNormal(Texture *image);
			UIAPI void SetImageHighlight(Texture *image);
			UIAPI void SetBackgroundColorNormal(const Color &color);
			UIAPI void SetBackgroundColorHighlight(const Color &color);
			UIAPI void SetTextColorNormal(const Color &color);
			UIAPI void SetTextColorHighlight(const Color &color);
			
			Label *GetLabel() const { return _label; }
			Texture *GetImageNormal() const { return _imageNormal; }
			Texture *GetImageHighlight() const { return _imageHighlight; }
			const Color &GetBackgroundColorNormal() const { return _backgroundColorNormal; }
			const Color &GetBackgroundColorHighlight() const { return _backgroundColorHighlight; }
			const Color &GetTextColorNormal() const { return _textColorNormal; }
			const Color &GetTextColorHighlight() const { return _textColorHighlight; }
			
			bool GetIsHighlighted() const { return _isHighlighted; }
			void SetIsHighlighted(bool isHighlighted);
			
		protected:
			UIAPI virtual bool UpdateCursorPosition(const Vector2 &cursorPosition) override;

		private:
			void UpdateForHighlight();
			
			Label *_label;
			
			Texture *_imageNormal;
			Texture *_imageHighlight;
			Color _backgroundColorNormal;
			Color _backgroundColorHighlight;
			Color _textColorNormal;
			Color _textColorHighlight;
			
			bool _isHighlighted;
			bool _wasHighlighted;

			RNDeclareMetaAPI(Button, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIBUTTON_H_ */
