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
#include "RNUIImage.h"
#include "RNUIFont.h"

namespace RN
{
	namespace UI
	{
		struct ContextInternals;
		class Label;

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

		enum StrokeCap
		{
			Butt,
			Round,
			Square
		};

		enum StrokeJoin
		{
			MiterJoin,
			RoundJoin,
			BevelJoin
		};

		class Context : public Object
		{
		public:
			UIAPI Context(size_t width, size_t height, bool alpha = false, bool mipmaps = false);
			UIAPI ~Context();

			UIAPI void SaveState();
			UIAPI void RestoreState();
			UIAPI void Clear(const Color &color);

			UIAPI void DrawImage(const Image *image, const Rect &rect);
			UIAPI void DrawTextRect(const String *text, const Rect &rect);
			UIAPI void DrawLabel(const Label *label);

			UIAPI void FillPath(const Path *path);
			UIAPI void FillPath(const Path *path, BlendMode blendMode, float alpha = 1.0);
			UIAPI void StrokePath(const Path *path);
			UIAPI void StrokePath(const Path *path, BlendMode blendMode, float alpha = 1.0);

			UIAPI void FillRect(const Rect &rect);
			UIAPI void StrokeRect(const Rect &rect);

			UIAPI void SetAntiAlias(bool antiAlias);
			UIAPI void SetClipRect(const Rect &rect);

			UIAPI void SetFillColor(const Color &color);
			UIAPI void SetStrokeColor(const Color &color);

			UIAPI void SetFont(Font *font);
			
			UIAPI void SetStrokeWidth(float strokeWidth);
			UIAPI void SetStrokeMiter(float strokeWidth);
			UIAPI void SetStrokeCap(StrokeCap cap);
			UIAPI void SetStrokeJoin(StrokeJoin join);

			UIAPI void Translate(const Vector2 &offset);
			UIAPI void Rotate(float degrees);

			Texture *GetTexture() const { return _texture; }
			UIAPI void UpdateTexture();

		private:
			PIMPL<ContextInternals> _internals;

			size_t _width;
			size_t _height;
			size_t _rowBytes;
			bool _hasAlpha;
			bool _hasMipmaps;

			Texture *_texture;

			RNDeclareMetaAPI(Context, UIAPI)
		};
	}
}

#endif /* __RAYNE_UICONTEXT_H_ */
