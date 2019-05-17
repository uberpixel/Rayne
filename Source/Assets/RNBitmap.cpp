//
//  RNBitmap.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBitmap.h"

namespace RN
{
	RNDefineMeta(Bitmap, Asset)

	size_t kBytesPerPixel[] = {
		4,
		3,

		2,
		2,
		2,

		12, // RGB32F
		16
	};

	Bitmap::Bitmap(const Bitmap *other) :
		_data(other->_data->Retain()),
		_info(other->_info),
		_bytesPerPixel(other->_bytesPerPixel)
	{}

	Bitmap::Bitmap(const uint8_t *bytes, const BitmapInfo &info) :
		_info(info)
	{
		_data = new Data(bytes, info.height * info.bytesPerRow);

		Update();
	}

	Bitmap::Bitmap(const Data *data, const BitmapInfo &info) :
		_data(data->Copy()),
		_info(info)
	{
		Update();
	}

	Bitmap::Bitmap(Data *data, const BitmapInfo &info, bool dummy) :
		_data(SafeRetain(data)),
		_info(info)
	{
		Update();
	}

	Bitmap::~Bitmap()
	{
		_data->Release();
	}

	void Bitmap::Update()
	{
		_bytesPerPixel = kBytesPerPixel[static_cast<size_t>(_info.format)];
	}


	Color Bitmap::GetPixel(size_t x, size_t y) const
	{
		Color pixel;
		size_t index = (y * _info.bytesPerRow) + (x *_bytesPerPixel);

		switch(_info.format)
		{
			case BitmapInfo::Format::RGBA_32F:
			{
				const float *data = _data->GetBytes<float>(index);
				pixel.r = data[0];
				pixel.g = data[1];
				pixel.b = data[2];
				pixel.a = data[3];

				break;
			}
			case BitmapInfo::Format::RGB_32F:
			{
				const float *data = _data->GetBytes<float>(index);
				pixel.r = data[0];
				pixel.g = data[1];
				pixel.b = data[2];
				pixel.a = 1.0f;

				break;
			}
			case BitmapInfo::Format::RGBA_8:
			{
				const uint32_t *data = _data->GetBytes<uint32_t>(index);

				pixel.r = ((*data >> 24) & 0xFF) / 255.0f;
				pixel.g = ((*data >> 16) & 0xFF) / 255.0f;
				pixel.b = ((*data >> 8) & 0xFF) / 255.0f;
				pixel.a = ((*data >> 0) & 0xFF) / 255.0f;

				break;
			}
			case BitmapInfo::Format::RGB_8:
			{
				const uint16 *dataRG = _data->GetBytes<uint16>(index);
				const uint8 *dataB = reinterpret_cast<const uint8 *>(dataRG + 1);

				pixel.r = ((*dataRG >> 0) & 0xFF) / 255.0f;
				pixel.g = ((*dataRG >> 8) & 0xFF) / 255.0f;
				pixel.b = (*dataB) / 255.0f;
				pixel.a = 1.0f;

				break;
			}
			case BitmapInfo::Format::RGBA_4:
			{
				const uint16_t *data = _data->GetBytes<uint16_t>(index);

				pixel.r = (((*data >> 0) & 0xF) << 4) / 255.0f;
				pixel.g = (((*data >> 4)  & 0xF) << 4) / 255.0f;
				pixel.b = (((*data >> 8)  & 0xF) << 4) / 255.0f;
				pixel.a = (((*data >> 12)  & 0xF) << 4) / 255.0f;

				break;
			}
			case BitmapInfo::Format::RGB_5_A_1:
			{
				const uint16_t *data = _data->GetBytes<uint16_t>(index);

				pixel.r = (((*data >> 0) & 0xF) << 3) / 255.0f;
				pixel.g = (((*data >> 1)  & 0xF) << 3) / 255.0f;
				pixel.b = (((*data >> 6)  & 0xF) << 3) / 255.0f;
				pixel.a = (((*data >> 11)  & 0xF) << 7) / 255.0f;

				break;
			}
			case BitmapInfo::Format::R_5_G_6_B_5:
			{
				const uint16_t *data = _data->GetBytes<uint16_t>(index);

				pixel.r = (((*data >> 0) & 0xF) << 3) / 255.0f;
				pixel.g = (((*data >> 5)  & 0xF) << 2) / 255.0f;
				pixel.b = (((*data >> 11)  & 0xF) << 3) / 255.0f;
				pixel.a = 1.0f;

				break;
			}
		}

		return pixel;
	}
	void Bitmap::SetPixel(size_t x, size_t y, const Color &pixel)
	{
		size_t index = (y * _info.bytesPerRow) + (x *_bytesPerPixel);

		switch(_info.format)
		{
			case BitmapInfo::Format::RGBA_32F:
			{
				float *data = _data->GetBytes<float>(index);

				data[0] = pixel.r;
				data[1] = pixel.g;
				data[2] = pixel.b;
				data[3] = pixel.a;

				break;
			}
			case BitmapInfo::Format::RGB_32F:
			{
				float *data = _data->GetBytes<float>(index);

				data[0] = pixel.r;
				data[1] = pixel.g;
				data[2] = pixel.b;

				break;
			}
			case BitmapInfo::Format::RGBA_8:
			{
				uint8_t *data = _data->GetBytes<uint8_t>(index);

				data[0] = static_cast<uint8>(pixel.r * 255);
				data[1] = static_cast<uint8>(pixel.g * 255);
				data[2] = static_cast<uint8>(pixel.b * 255);
				data[3] = static_cast<uint8>(pixel.a * 255);

				break;
			}
			case BitmapInfo::Format::RGB_8:
			{
				uint8_t *data = _data->GetBytes<uint8_t>(index);

				data[0] = static_cast<uint8>(pixel.r * 255);
				data[1] = static_cast<uint8>(pixel.g * 255);
				data[2] = static_cast<uint8>(pixel.b * 255);

				break;
			}
			case BitmapInfo::Format::RGBA_4:
			{
				uint16_t *data = _data->GetBytes<uint16_t>(index);

				data[0] = static_cast<uint8>(pixel.r * 255) >> 4;
				data[1] = static_cast<uint8>(pixel.g * 255) >> 4;
				data[2] = static_cast<uint8>(pixel.b * 255) >> 4;
				data[3] = static_cast<uint8>(pixel.a * 255) >> 4;

				break;
			}
			case BitmapInfo::Format::RGB_5_A_1:
			{
				uint16_t *data = _data->GetBytes<uint16_t>(index);

				data[0] = static_cast<uint8>(pixel.r * 255) >> 3;
				data[1] = static_cast<uint8>(pixel.g * 255) >> 3;
				data[2] = static_cast<uint8>(pixel.b * 255) >> 3;
				data[3] = static_cast<uint8>(pixel.a * 255) >> 7;

				break;
			}
			case BitmapInfo::Format::R_5_G_6_B_5:
			{
				uint16_t *data = _data->GetBytes<uint16_t>(index);

				data[0] = static_cast<uint8>(pixel.r * 255) >> 3;
				data[1] = static_cast<uint8>(pixel.g * 255) >> 2;
				data[2] = static_cast<uint8>(pixel.b * 255) >> 3;

				break;
			}
		}
	}

	Bitmap *Bitmap::GetBitmapWithFormat(BitmapInfo::Format format, size_t bytesPerRow) const
	{
		if(bytesPerRow == 0)
			bytesPerRow = kBytesPerPixel[static_cast<size_t>(format)] * _info.width;

		if(bytesPerRow == _info.bytesPerRow && format == _info.format)
		{
			Bitmap *bitmap = new Bitmap(this);
			return bitmap->Autorelease();
		}

		BitmapInfo info(_info);
		info.format = format;
		info.bytesPerRow = bytesPerRow;

		Data *data = new Data(nullptr, info.bytesPerRow * info.height);
		Bitmap *other = new Bitmap(data, info, true);

		for(size_t y = 0; y < _info.height; y ++)
		{
			for(size_t x = 0; x < _info.width; x++)
			{
				Color pixel = GetPixel(x, y);
				other->SetPixel(x, y, pixel);
			}
		}

		data->Release();

		return other->Autorelease();
	}
}
