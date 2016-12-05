//
//  RNUIContext.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICONTEXT_H_
#define __RAYNE_UICONTEXT_H_

#include "RNUIConfig.h"
#include "RNUIPath.h"

namespace RN
{
	namespace UI
	{
		struct ContextInternals;

		enum class BlendMode
		{
			Clear,
			Source,
			Destination,
			SourceOver,
			DestinationOver,
			SourceIn,
			DestinationIn,
			SourceOut,
			DestinationOut,
			SourceATop,
			DestinationATop,
			XOR,
			Plus,
			Modulate,
			Screen,

			Overlay,
			Darken,
			Lighten,
			ColorDodge,
			ColorBur,
			HardLight,
			SoftLight,
			Difference,
			Exclusion,
			Multiply,

			Hue,
			Saturation,
			Color,
			Luminosity
		};

		class Context : public Object
		{
		public:
			UIAPI Context(size_t width, size_t height, bool alpha = false);
			UIAPI ~Context();

			UIAPI void SaveState();
			UIAPI void RestoreState();
			UIAPI void Clear(const Color &color);

			UIAPI void FillPath(const Path *path);
			UIAPI void FillPath(const Path *path, BlendMode blendMode, float alpha = 1.0);
			UIAPI void StrokePath(const Path *path);
			UIAPI void StrokePath(const Path *path, BlendMode blendMode, float alpha = 1.0);

			UIAPI void SetFillColor(const Color &color);
			UIAPI void SetStrokeColor(const Color &color);

			UIAPI void SetStrokeWidth(float strokeWidth);
			UIAPI void SetStrokeMiter(float strokeWidth);

			UIAPI Texture *GetTexture(bool generateMipMaps = false) const;

		private:
			PIMPL<ContextInternals> _internals;

			size_t _width;
			size_t _height;
			size_t _rowBytes;
			bool _hasAlpha;

			RNDeclareMetaAPI(Context, UIAPI)
		};
	}
}

#endif /* __RAYNE_UICONTEXT_H_ */
