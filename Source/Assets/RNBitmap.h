//
//  RNBitmap.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BITMAP_H_
#define __RAYNE_BITMAP_H_

#include "../Base/RNBase.h"
#include "../Assets/RNAsset.h"
#include "../Objects/RNData.h"

namespace RN
{
	struct BitmapInfo
	{
		enum class Format
		{
			RGBA8888,
			RGB888,

			RGBA4444,
			RGBA5551,
			RGB565,

			RGB32F,
			RGBA32F
		};

		BitmapInfo() = default;

		BitmapInfo(const BitmapInfo &other) = default;
		BitmapInfo &operator =(const BitmapInfo &other) = default;

		Format format;
		size_t width;
		size_t height;
		size_t bytesPerRow;
	};

	class Bitmap : public Asset
	{
	public:
		RNAPI Bitmap(const Bitmap *other);
		RNAPI Bitmap(const uint8_t *bytes, const BitmapInfo &info);
		RNAPI Bitmap(const Data *data, const BitmapInfo &info);

		RNAPI ~Bitmap();

		RNAPI Bitmap *GetBitmapWithFormat(BitmapInfo::Format format, size_t bytesPerRow = 0) const;

		size_t GetWidth() const { return _info.width; }
		size_t GetHeight() const { return _info.height; }
		const BitmapInfo &GetInfo() const { return _info; }

		const Data *GetData() const { return _data; }

	private:
		struct Pixel
		{
			float r;
			float g;
			float b;
			float a;

			uint8 GetRed() const
			{
				return static_cast<uint8>(r * 255);
			}
			uint8 GetGreen() const
			{
				return static_cast<uint8>(g * 255);
			}
			uint8 GetBlue() const
			{
				return static_cast<uint8>(b * 255);
			}
			uint8 GetAlpha() const
			{
				return static_cast<uint8>(a * 255);
			}
		};

		Bitmap(Data *data, const BitmapInfo &info, bool dummy);

		void ReadPixel(size_t x, size_t y, Pixel &pixel) const;
		void WritePixel(size_t x, size_t y, const Pixel &pixel);

		void Update();

		Data *_data;
		BitmapInfo _info;
		size_t _bytesPerPixel;

		__RNDeclareMetaInternal(Bitmap)
	};
}


#endif /* __RAYNE_BITMAP_H_ */
