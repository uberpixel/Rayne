//
//  RNUIImage.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImage.h"
#include "RNUIInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Image, Object)

		Image::Image(const Bitmap *bitmap)
		{
			SkColorType colorType;

			switch(bitmap->GetInfo().format)
			{
				case BitmapInfo::Format::RGBA_8:
					colorType = kRGBA_8888_SkColorType;
					break;
				case BitmapInfo::Format::R_5_G_6_B_5:
					colorType = kRGB_565_SkColorType;
					break;

				default:
					colorType = kRGBA_8888_SkColorType;
					bitmap = bitmap->GetBitmapWithFormat(BitmapInfo::Format::RGBA_8);
					break;

			}

			SkImageInfo info = SkImageInfo::Make(bitmap->GetWidth(), bitmap->GetHeight(), colorType, kPremul_SkAlphaType, nullptr);
			SkPixmap pixmap(info, bitmap->GetData()->GetBytes(), bitmap->GetInfo().bytesPerRow);

			_internals->image = SkImage::MakeRasterCopy(pixmap);
		}

		Image *Image::WithName(const String *name)
		{
			AssetManager *coordinator = AssetManager::GetSharedInstance();
			Bitmap *bitmap = coordinator->GetAssetWithName<Bitmap>(name, nullptr);

			Image *image = new Image(bitmap);
			return image->Autorelease();
		}
	}
}
