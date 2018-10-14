//
//  RNUIContext.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIContext.h"
#include "RNUIInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Context, Object)

		Context::Context(size_t width, size_t height, bool alpha) :
			_width(width),
			_height(height),
			_hasAlpha(alpha)
		{
			SkImageInfo info = SkImageInfo::MakeN32(static_cast<int>(width), static_cast<int>(height), alpha ? kPremul_SkAlphaType : kOpaque_SkAlphaType);
			_rowBytes = info.minRowBytes();

			_internals->backingSurface.resize(info.getSafeSize(_rowBytes));
			_internals->surface = SkSurface::MakeRasterDirect(info, _internals->backingSurface.data(), _rowBytes);

			_internals->strokeStyle.setStyle(SkPaint::kStroke_Style);
			_internals->fillStyle.setStyle(SkPaint::kFill_Style);

			Texture::Format format = _hasAlpha ? Texture::Format::RGBA8888SRGB : Texture::Format::RGB888SRGB;
			Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(format, _width, _height, false);
			_texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);

		}
		Context::~Context()
		{
			_texture->Release();
		}


		void Context::SaveState()
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->save();

			_internals->restoreStyles.push_back(std::make_pair(_internals->fillStyle, _internals->strokeStyle));
		}
		void Context::RestoreState()
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->restore();

			auto &pair = _internals->restoreStyles.back();
			_internals->fillStyle = pair.first;
			_internals->strokeStyle = pair.second;

			_internals->restoreStyles.pop_back();
		}
		void Context::Clear(const Color &color)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->clear(MakeColor(color));
		}


		void Context::SetAntiAlias(bool antiAlias)
		{
			_internals->strokeStyle.setAntiAlias(antiAlias);
			_internals->fillStyle.setAntiAlias(antiAlias);
		}
		void Context::SetClipRect(const Rect &rect)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->clipRect(MakeRect(rect));
		}


		void Context::SetFillColor(const Color &color)
		{
			_internals->fillStyle.setColor(MakeColor(color));
		}
		void Context::SetStrokeColor(const Color &color)
		{
			_internals->strokeStyle.setColor(MakeColor(color));
		}

		void Context::SetStrokeWidth(float strokeWidth)
		{
			_internals->strokeStyle.setStrokeWidth(strokeWidth);
		}
		void Context::SetStrokeMiter(float strokeWidth)
		{
			_internals->strokeStyle.setStrokeWidth(strokeWidth);
		}

		void Context::SetStrokeCap(StrokeCap cap)
		{
			_internals->strokeStyle.setStrokeCap(static_cast<SkPaint::Cap>(cap));
		}
		void Context::SetStrokeJoin(StrokeJoin join)
		{
			_internals->strokeStyle.setStrokeJoin(static_cast<SkPaint::Join >(join));
		}

		void Context::FillPath(const Path *path)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->drawPath(path->_internals->path, _internals->fillStyle);
		}
		void Context::FillPath(const Path *path, BlendMode blendMode, float alpha)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			SkBlendMode previous = _internals->strokeStyle.getBlendMode();
			U8CPU previousAlpha = _internals->strokeStyle.getAlpha();

			_internals->fillStyle.setBlendMode(static_cast<SkBlendMode>(blendMode));
			_internals->fillStyle.setAlpha(static_cast<U8CPU>(alpha * 255));

			canvas->drawPath(path->_internals->path, _internals->fillStyle);

			_internals->fillStyle.setBlendMode(previous);
			_internals->fillStyle.setAlpha(previousAlpha);
		}
		void Context::StrokePath(const Path *path)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->drawPath(path->_internals->path, _internals->strokeStyle);
		}
		void Context::StrokePath(const Path *path, BlendMode blendMode, float alpha)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			SkBlendMode previous = _internals->strokeStyle.getBlendMode();
			U8CPU previousAlpha = _internals->strokeStyle.getAlpha();

			_internals->strokeStyle.setBlendMode(static_cast<SkBlendMode>(blendMode));
			_internals->strokeStyle.setAlpha(static_cast<U8CPU>(alpha * 255));

			canvas->drawPath(path->_internals->path, _internals->strokeStyle);

			_internals->strokeStyle.setBlendMode(previous);
			_internals->strokeStyle.setAlpha(previousAlpha);
		}

		void Context::FillRect(const Rect &rect)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->drawRect(MakeRect(rect), _internals->fillStyle);
		}
		void Context::StrokeRect(const Rect &rect)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->drawRect(MakeRect(rect), _internals->strokeStyle);
		}

		void Context::DrawImage(const Image *image, const Rect &rect)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->drawImageRect(image->_internals->image, MakeRect(rect), &_internals->fillStyle);
		}


		void Context::Translate(const Vector2 &offset)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->translate(offset.x, offset.y);
		}
		void Context::Rotate(float degrees)
		{
			SkCanvas *canvas = _internals->surface->getCanvas();
			canvas->rotate(degrees);
		}


		void Context::UpdateTexture(bool generateMipMaps)
		{
			_texture->SetData(0, _internals->backingSurface.data(), _rowBytes);

			if(generateMipMaps)
				_texture->GenerateMipMaps();
		}
	}
}
