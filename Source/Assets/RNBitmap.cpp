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

	Bitmap::Bitmap(Data *data, const BitmapInfo &info, __unused bool dummy) :
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


	void Bitmap::ReadPixel(size_t x, size_t y, Pixel &pixel) const
	{
		size_t index = (y * _info.bytesPerRow) + (x *_bytesPerPixel);

		switch(_info.format)
		{
			case BitmapInfo::Format::RGBA32F:
			{
				const float *data = _data->GetBytes<float>(index);
				pixel.r = data[0];
				pixel.g = data[1];
				pixel.b = data[2];
				pixel.a = data[3];

				break;
			}
			case BitmapInfo::Format::RGB32F:
			{
				const float *data = _data->GetBytes<float>(index);
				pixel.r = data[0];
				pixel.g = data[1];
				pixel.b = data[2];
				pixel.a = 1.0f;

				break;
			}
			case BitmapInfo::Format::RGBA8888:
			{
				const uint32_t *data = _data->GetBytes<uint32_t>(index);

				pixel.r = ((*data >> 24) & 0xFF) / 255.0f;
				pixel.g = ((*data >> 16) & 0xFF) / 255.0f;
				pixel.b = ((*data >> 8) & 0xFF) / 255.0f;
				pixel.a = ((*data >> 0) & 0xFF) / 255.0f;

				break;
			}
			case BitmapInfo::Format::RGB888:
			{
				const uint16 *dataRG = _data->GetBytes<uint16>(index);
				const uint8 *dataB = reinterpret_cast<const uint8 *>(dataRG + 1);

				pixel.r = ((*dataRG >> 0) & 0xFF) / 255.0f;
				pixel.g = ((*dataRG >> 8) & 0xFF) / 255.0f;
				pixel.b = (*dataB) / 255.0f;
				pixel.a = 1.0f;

				break;
			}
			case BitmapInfo::Format::RGBA4444:
			{
				const uint16_t *data = _data->GetBytes<uint16_t>(index);

				pixel.r = (((*data >> 0) & 0xF) << 4) / 255.0f;
				pixel.g = (((*data >> 4)  & 0xF) << 4) / 255.0f;
				pixel.b = (((*data >> 8)  & 0xF) << 4) / 255.0f;
				pixel.a = (((*data >> 12)  & 0xF) << 4) / 255.0f;

				break;
			}
			case BitmapInfo::Format::RGBA5551:
			{
				const uint16_t *data = _data->GetBytes<uint16_t>(index);

				pixel.r = (((*data >> 0) & 0xF) << 3) / 255.0f;
				pixel.g = (((*data >> 1)  & 0xF) << 3) / 255.0f;
				pixel.b = (((*data >> 6)  & 0xF) << 3) / 255.0f;
				pixel.a = (((*data >> 11)  & 0xF) << 7) / 255.0f;

				break;
			}
			case BitmapInfo::Format::RGB565:
			{
				const uint16_t *data = _data->GetBytes<uint16_t>(index);

				pixel.r = (((*data >> 0) & 0xF) << 3) / 255.0f;
				pixel.g = (((*data >> 5)  & 0xF) << 2) / 255.0f;
				pixel.b = (((*data >> 11)  & 0xF) << 3) / 255.0f;
				pixel.a = 1.0f;

				break;
			}
		}
	}
	void Bitmap::WritePixel(size_t x, size_t y, const Pixel &pixel)
	{
		size_t index = (y * _info.bytesPerRow) + (x *_bytesPerPixel);

		switch(_info.format)
		{
			case BitmapInfo::Format::RGBA32F:
			{
				float *data = _data->GetBytes<float>(index);

				data[0] = pixel.r;
				data[1] = pixel.g;
				data[2] = pixel.b;
				data[3] = pixel.a;

				break;
			}
			case BitmapInfo::Format::RGB32F:
			{
				float *data = _data->GetBytes<float>(index);

				data[0] = pixel.r;
				data[1] = pixel.g;
				data[2] = pixel.b;

				break;
			}
			case BitmapInfo::Format::RGBA8888:
			{
				uint8_t *data = _data->GetBytes<uint8_t>(index);

				data[0] = pixel.GetRed();
				data[1] = pixel.GetGreen();
				data[2] = pixel.GetBlue();
				data[3] = pixel.GetAlpha();

				break;
			}
			case BitmapInfo::Format::RGB888:
			{
				uint8_t *data = _data->GetBytes<uint8_t>(index);

				data[0] = pixel.GetRed();
				data[1] = pixel.GetGreen();
				data[2] = pixel.GetBlue();

				break;
			}
			case BitmapInfo::Format::RGBA4444:
			{
				uint16_t *data = _data->GetBytes<uint16_t>(index);

				data[0] = pixel.GetRed() >> 4;
				data[1] = pixel.GetGreen() >> 4;
				data[2] = pixel.GetBlue() >> 4;
				data[3] = pixel.GetAlpha() >> 4;

				break;
			}
			case BitmapInfo::Format::RGBA5551:
			{
				uint16_t *data = _data->GetBytes<uint16_t>(index);

				data[0] = pixel.GetRed() >> 3;
				data[1] = pixel.GetGreen() >> 3;
				data[2] = pixel.GetBlue() >> 3;
				data[3] = pixel.GetAlpha() >> 7;

				break;
			}
			case BitmapInfo::Format::RGB565:
			{
				uint16_t *data = _data->GetBytes<uint16_t>(index);

				data[0] = pixel.GetRed() >> 3;
				data[1] = pixel.GetGreen() >> 2;
				data[2] = pixel.GetBlue() >> 3;

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
				Pixel pixel;

				ReadPixel(x, y, pixel);
				other->WritePixel(x, y, pixel);
			}
		}

		data->Release();

		return other->Autorelease();
	}
}
