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
#include "../Math/RNColor.h"

namespace RN
{
	struct BitmapInfo
	{
		enum class Format
		{
			Invalid,
			
			RGBA_8,
			RGB_8,

			RGBA_4,
			RGB_5_A_1,
			R_5_G_6_B_5,

			RGB_32F,
			RGBA_32F
		};

		BitmapInfo() = default;

		BitmapInfo(const BitmapInfo &other) = default;
		BitmapInfo &operator =(const BitmapInfo &other) = default;

		Format format;
		size_t width;
		size_t height;
		size_t bytesPerRow;
		
		bool isTransposed;
	};

	class Bitmap : public Asset
	{
	public:
		RNAPI Bitmap(const Bitmap *other);
		RNAPI Bitmap(const uint8_t *bytes, const BitmapInfo &info);
		RNAPI Bitmap(const Data *data, const BitmapInfo &info);
		RNAPI Bitmap(Data *data, const BitmapInfo &info);
		
		RNAPI static Bitmap *WithName(const String *name, const Dictionary *settings = nullptr);

		RNAPI ~Bitmap();

		RNAPI Bitmap *GetBitmapWithFormat(BitmapInfo::Format format, size_t bytesPerRow = 0) const;
		RNAPI Bitmap *GetScaledBitmap(size_t newWidth, size_t newHeight) const;

		Color GetPixel(size_t x, size_t y) const;
		void SetPixel(size_t x, size_t y, const Color &color);

		size_t GetWidth() const { return _info.width; }
		size_t GetHeight() const { return _info.height; }
		const BitmapInfo &GetInfo() const { return _info; }

		Data *GetData() const { return _data; }

	private:
		//Bitmap(Data *data, const BitmapInfo &info, bool dummy);

		void Update();

		Data *_data;
		BitmapInfo _info;
		size_t _bytesPerPixel;

		__RNDeclareMetaInternal(Bitmap)
	};
}


#endif /* __RAYNE_BITMAP_H_ */
