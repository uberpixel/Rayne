//
//  RNBitmap.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBitmap.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(Bitmap, Asset)

	size_t kBytesPerPixel[] = {
		0,

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

	Bitmap::Bitmap(Data *data, const BitmapInfo &info) :
		_data(SafeRetain(data)),
		_info(info)
	{
		Update();
	}

	Bitmap *Bitmap::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Bitmap>(name, settings);
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
				const uint8_t *data = _data->GetBytes<uint8_t>(index);

				pixel.r = data[0] / 255.0f;
				pixel.g = data[1] / 255.0f;
				pixel.b = data[2] / 255.0f;
				pixel.a = data[3] / 255.0f;

				break;
			}
			case BitmapInfo::Format::RGB_8:
			{
				const uint8_t *data = _data->GetBytes<uint8_t>(index);

				pixel.r = data[0] / 255.0f;
				pixel.g = data[1] / 255.0f;
				pixel.b = data[2] / 255.0f;
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
				
			case BitmapInfo::Format::Invalid:
			{
				pixel.r = 0.0f;
				pixel.g = 0.0f;
				pixel.b = 0.0f;
				pixel.a = 0.0f;
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
			case BitmapInfo::Format::Invalid:
			{
				break;
			}
		}
	}

	Bitmap *Bitmap::GetScaledBitmap(size_t newWidth, size_t newHeight) const
	{
		RN_ASSERT(newWidth <= _info.width && newHeight <= _info.height, "Bitmap scaling only supports downscaling at the moment");
		
		const Bitmap *source = this->Retain();
		size_t tempWidth = source->_info.width;
		size_t tempHeight = source->_info.height;
		
		Bitmap *scaled = nullptr;
		
		while(tempWidth != newWidth || tempHeight != newHeight)
		{
			tempWidth = std::max(tempWidth / 2, newWidth);
			tempHeight = std::max(tempHeight / 2, newHeight);
			
			BitmapInfo scaledInfo;
			scaledInfo.width = tempWidth;
			scaledInfo.height = tempHeight;
			scaledInfo.format = _info.format;
			scaledInfo.bytesPerRow = tempWidth * kBytesPerPixel[static_cast<size_t>(_info.format)];
			
			Data *scaledData = new Data(scaledInfo.bytesPerRow * scaledInfo.height);
			scaled = new Bitmap(scaledData, scaledInfo);
			scaledData->Release();
			
			Color newColor;
			
			int l;
			int c;
			float t;
			float u;
			float tmp;
			float d1, d2, d3, d4;
			Color p1, p2, p3, p4; /* nearby pixels */
			
			for(int y = 0; y < tempHeight; y++)
			{
				for(int x = 0; x < tempWidth; x++)
				{
					tmp = (float) (x + 0.25f) / (float) (tempWidth - 1) * (source->_info.width - 1);
					c = std::min(std::max(static_cast<int>(floor(tmp)), 0), static_cast<int>(source->_info.width - 2));
					t = tmp - c;
					
					tmp = (float) (y + 0.25f) / (float)(tempHeight - 1) * (source->_info.height - 1);
					l = std::min(std::max(static_cast<int>(floor(tmp)), 0), static_cast<int>(source->_info.height - 2));
					u = tmp - l;
					
					//coefficients
					d1 = (1 - t) * (1 - u);
					d2 = (1 - t) * u;
					d3 = t * u;
					d4 = t * (1 - u);
					
					//nearby pixels
					p1 = source->GetPixel(c, l);
					p2 = source->GetPixel(c, l + 1);
					p3 = source->GetPixel(c + 1, l + 1);
					p4 = source->GetPixel(c + 1, l);
					
					newColor = p1 * d1 + p2 * d2 + p3 * d3 + p4 * d4;
					
					scaled->SetPixel(x, y, newColor);
				}
			}
			
			source->Release();
			source = scaled;
		}
		
		return scaled->Autorelease();
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
		Bitmap *other = new Bitmap(data, info);

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
