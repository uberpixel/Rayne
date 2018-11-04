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

namespace RN
{
	namespace UI
	{
		struct LabelInternals;
		
		class Label : public View
		{
		public:
			enum Alignment
			{
				Left,
				Center,
				Right
			};
			
			friend Context;
			UIAPI Label();
			UIAPI Label(const Rect &frame);
			UIAPI ~Label();
			
			UIAPI void SetText(String *text);
			UIAPI void SetColor(Color color);
			UIAPI void SetFont(Font *font);
			UIAPI void SetAlignment(Alignment alignment);
			
			UIAPI virtual void Draw(Context *context) const override;

		protected:
			UIAPI virtual void LayoutSubviews() override;

		private:
			String *_text;
			Color _color;
			Font *_font;
			bool _needsShaping;
			Alignment _alignment;
			
			PIMPL<LabelInternals> _internals;

			RNDeclareMetaAPI(Label, UIAPI)
		};
	}
}


#endif /* __RAYNE_UILABEL_H_ */