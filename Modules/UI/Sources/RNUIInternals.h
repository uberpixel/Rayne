//
//  RNUIInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIINTERNALS_H_
#define __RAYNE_UIINTERNALS_H_

//#include <skia.h>
#include <include/core/SkGraphics.h>
#include <include/core/SkPaint.h>
#include <include/core/SkSurface.h>
#include <include/core/SkPath.h>
#include <include/core/SkImage.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkFontMetrics.h>
#include <include/core/SkTypeface.h>
#include <include/core/SkTextBlob.h>
//#include <include/modules/SkShaper.h>
#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		struct ContextInternals
		{
			sk_sp<SkSurface> surface;
			SkPaint strokeStyle;
			SkPaint fillStyle;
			std::vector<std::pair<SkPaint, SkPaint>> restoreStyles;
			std::vector<char> backingSurface;
		};

		struct PathInternals
		{
			SkPath path;
		};

		struct ImageInternals
		{
			sk_sp<SkImage> image;
		};
		
		struct FontInternals
		{
			SkFont font;
			//SkShaper *shaper;
		};
		
		struct LabelInternals
		{
			SkPaint style;
			//SkTextBlobBuilder builder;
			//sk_sp<SkTextBlob> textBlob;
		};



		static SkColor MakeColor(const Color &color)
		{
			SkColor4f intermediate;
			intermediate.fR = color.r;
			intermediate.fG = color.g;
			intermediate.fB = color.b;
			intermediate.fA = color.a;

			return intermediate.toSkColor();
		}
		static SkRect MakeRect(const Rect &rect)
		{
			return SkRect::MakeXYWH(rect.x, rect.y, rect.width, rect.height);
		}
	}
}


#endif /* __RAYNE_UIINTERNALS_H_ */
