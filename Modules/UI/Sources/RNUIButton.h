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
			UIAPI Button();
			UIAPI ~Button();

			UIAPI void SetImageNormal(Image *image);
			UIAPI void SetImageHighlight(Image *image);
			UIAPI void SetBackgroundColorNormal(const Color &color);
			UIAPI void SetBackgroundColorHighlight(const Color &color);
			UIAPI void SetTextColorNormal(const Color &color);
			UIAPI void SetTextColorHighlight(const Color &color);
			
			Label *GetLabel() const { return _label; }
			Image *GetImageNormal() const { return _imageNormal; }
			Image *GetImageHighlight() const { return _imageHighlight; }
			const Color &GetBackgroundColorNormal() const { return _backgroundColorNormal; }
			const Color &GetBackgroundColorHighlight() const { return _backgroundColorHighlight; }
			const Color &GetTextColorNormal() const { return _textColorNormal; }
			const Color &GetTextColorHighlight() const { return _textColorHighlight; }
			
			bool GetIsHighlighted() const { return _isHighlighted; }
			
		protected:
			UIAPI virtual void LayoutSubviews() override;
			UIAPI virtual void UpdateCursorPosition(const Vector2 &cursorPosition) override;

		private:
			Label *_label;
			
			Image *_imageNormal;
			Image *_imageHighlight;
			Color _backgroundColorNormal;
			Color _backgroundColorHighlight;
			Color _textColorNormal;
			Color _textColorHighlight;
			
			bool _isHighlighted;

			RNDeclareMetaAPI(Button, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIBUTTON_H_ */
