//
//  RNData.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#include "../Base/RNBase.h"
#include <fcntl.h>
#if RN_PLATFORM_POSIX
	#include <unistd.h>
	#define O_BINARY 0x0
#elif RN_PLATFORM_WINDOWS
	#include "../Base/RNUnistd.h"
#endif
#if RN_PLATFORM_ANDROID
	#include "../Base/RNKernel.h"
#endif

#include <sys/stat.h>

#include "RNData.h"
#include "RNSerialization.h"
#include "RNString.h"
#include "../System/RNFileManager.h"

#define kRNDataIncreaseLength 64
#define kRNDataReadBufferSize 1024

namespace RN
{
	RNDefineMeta(Data, Object)
	
	Data::Data()
	{
		_length = 0;
		_allocated = 10;
		_bytes = (uint8 *)malloc(_allocated);
		
		_ownsData = _freeData = true;
	}
	
	Data::Data(const void *bytes, size_t length) :
		Data(bytes, length, false, true)
	{
	}
	
	Data::Data(const void *bytes, size_t length, bool noCopy, bool deleteWhenDone)
	{
		if(noCopy)
		{
			_bytes = const_cast<uint8 *>(static_cast<const uint8 *>(bytes));
			_allocated = 0;
			_length    = length;
			
			_ownsData = false;
			_freeData = deleteWhenDone;
		}
		else
		{
			Initialize(bytes, length);
		}
	}

	
	Data::Data(const Data *other)
	{
		Initialize(other->_bytes, other->_length);
	}
	
	Data::~Data()
	{
		if(_freeData)
			free(_bytes);
	}
	
	
	
	Data::Data(Deserializer *deserializer)
	{
		uint8 *data = static_cast<uint8 *>(deserializer->DecodeBytes(&_length));
		
		_allocated = _length;
		_freeData  = _ownsData = true;
		_bytes     = (uint8 *)malloc(_allocated);
		
		std::copy(data, data + _length, _bytes);
	}
	
	void Data::Serialize(Serializer *serializer) const
	{
		serializer->EncodeBytes(_bytes, _length);
	}
	
	
	
	Data *Data::WithBytes(const uint8 *bytes, size_t length)
	{
		Data *data = new Data(bytes, length);
		return data->Autorelease();
	}

	Expected<Data *> Data::WithContentsOfFile(const String *file)
	{
#if RN_PLATFORM_ANDROID
		//TODO: Extract folder structure from app bundle instead and fix path resolving...
		if(file->HasPrefix(RNCSTR(":RayneVulkan:")))
		{
			String *tempFileName = file->GetSubstring(Range(13, file->GetLength()-13));
			file = RNSTR(RNCSTR("Resources/Modules/RayneVulkan/Resources") << tempFileName);
		}

		android_app *app = Kernel::GetSharedInstance()->GetAndroidApp();
		AAsset *asset = AAssetManager_open(app->activity->assetManager, file->GetUTF8String(), 0);
		if(!asset)
		{
			RN::String *tempfile = RNSTR(RNCSTR("Resources/") << file);
			asset = AAssetManager_open(app->activity->assetManager, tempfile->GetUTF8String(), 0);
		}

		if(asset)
		{
			off_t size = AAsset_getLength(asset);
			uint8 *bytes = (uint8 *)malloc(size);
			size_t bytesRead = 0;

			while(bytesRead < size)
			{
				ssize_t result = AAsset_read(asset, bytes + bytesRead, size - bytesRead);
				if(result < 0)
				{
					if(errno == EINTR)
						continue;

					AAsset_close(asset);
					return InconsistencyException("Failed to read file");
				}
				else
					bytesRead += result;
			}

			AAsset_close(asset);
			Data *data = new Data(bytes, size, true, true);
            return data->Autorelease();
		}
#endif

		String *path = FileManager::GetSharedInstance()->ResolveFullPath(file, 0);
		if(!path)
			return InvalidArgumentException(RNSTR("Couldn't open file " << file));

		int fd = open(path->GetUTF8String(), O_RDONLY|O_BINARY);
		if(fd < 0)
			return InvalidArgumentException(RNSTR("Couldn't open file " << path));

		off_t size = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);

		uint8 *bytes = (uint8 *)malloc(size);
		size_t bytesRead = 0;

		while(bytesRead < size)
		{
			ssize_t result = read(fd, bytes + bytesRead, size - bytesRead);
			if(result < 0)
			{
				if(errno == EINTR)
					continue;

				close(fd);
				return InconsistencyException("Failed to read file");
			}
			else
				bytesRead += result;
		}

		close(fd);

		Data *data = new Data(bytes, size, true, true);
		return data->Autorelease();
	}

	
	
	void Data::Initialize(const void *bytes, size_t length)
	{
		_ownsData = true;
		_freeData = true;
		
		_allocated = length;
		_length    = length;
		
		_bytes = (uint8 *)malloc(_allocated);
		
		if(bytes)
		{
			const uint8 *data = static_cast<const uint8 *>(bytes);
			std::copy(data, data + _length, _bytes);
		}
	}
	
	void Data::AssertSize(size_t minimumLength)
	{
		if(!_ownsData)
			throw InconsistencyException("The Data object doesn't own it's data and can't modify it!");
		
		if(_allocated < minimumLength)
		{
			uint8 *temp = (uint8 *)malloc(minimumLength + kRNDataIncreaseLength);
			std::copy(_bytes, _bytes + _length, temp);
			free(_bytes);
			
			_bytes = temp;
			_allocated = minimumLength + kRNDataIncreaseLength;
		}
	}
	
	
	void Data::Append(const void *bytes, size_t length)
	{
		AssertSize(_length + length);
		const uint8 *data = static_cast<const uint8 *>(bytes);
		
		std::copy(data, data + length, _bytes + _length);
		_length += length;
	}
	
	void Data::Append(Data *other)
	{
		Append(other->_bytes, other->_length);
	}
	
	void Data::ReplaceBytes(const void *bytes, const Range &range)
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		const uint8 *data = static_cast<const uint8 *>(bytes);
		std::copy(data, data + range.length, _bytes + range.origin);
	}
	
	bool Data::WriteToFile(const String *file)
	{
		String *path = FileManager::GetSharedInstance()->ResolveFullPath(file, FileManager::ResolveHint::CreateNode);
		if(!path)
			return false;

		int fd = open(path->GetUTF8String(), O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0644);
		if(fd < 0)
			return false;

		chmod(path->GetUTF8String(), 0755);

		size_t written = 0;
		while(written < _length)
		{
			ssize_t result = write(fd, _bytes + written, _length - written);
			if(result < 0)
			{
				if(errno == EINTR)
					continue;

				close(fd);
				return false;
			}

			written += result;
		}

		close(fd);
		return true;
	}
	
	
	Data *Data::GetDataInRange(Range range) const
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		uint8 *data = (uint8 *)malloc(range.length);
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
		
		Data *temp = new Data(data, range.length, true, true);
		return temp->Autorelease();
	}
	
	void Data::GetBytesInRange(void *buffer, Range range) const
	{
		if(range.origin + range.length > _length)
			throw RangeException("range is not within the datas bounds!");
		
		uint8 *data = static_cast<uint8 *>(buffer);
		std::copy(_bytes + range.origin, _bytes + range.origin + range.length, data);
	}
}
