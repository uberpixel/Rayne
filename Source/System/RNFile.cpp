//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"
#include "RNFileManager.h"

#if RN_PLATFORM_WINDOWS
	#include "../Base/RNUnistd.h"
#elif RN_PLATFORM_POSIX
	#include <unistd.h>
	#include <fcntl.h>
#define O_BINARY 0x0
#endif

namespace RN
{
	RNDefineMeta(File, Object)

	RNExceptionImp(FileNotFound)
	RNExceptionImp(FileIO)
	RNExceptionImp(FileGeneric)

	File::File(int fd, const String *path, Mode mode) :
		_fd(fd),
		_mode(mode),
		_path(path->Copy())
	{
		_size = static_cast<size_t>(lseek(fd, 0, SEEK_END));
		lseek(fd, 0, SEEK_SET);
	}

	File::~File()
	{
		close(_fd);
		SafeRelease(_path);
	}


	size_t File::GetOffset() const
	{
		return static_cast<size_t>(lseek(_fd, 0, SEEK_CUR));
	}
	void File::Seek(size_t offset, bool fromStart)
	{
		lseek(_fd, static_cast<off_t>(offset), fromStart ? SEEK_SET : SEEK_CUR);
	}


	size_t File::Read(void *buffer, size_t size)
	{
		RN_ASSERT(_mode & Mode::Read, "Trying to read from a file not opened for reading");

		size_t totalRead = 0;
		size_t left = size;
		uint8 *bytes = static_cast<uint8 *>(buffer);

		int error = errno;
		ScopeGuard guard([=]{
			errno = error;
		});

		do {

			ssize_t bytesRead = read(_fd, bytes + totalRead, left);
			if(bytesRead == 0)
				break;

			if(bytesRead == -1)
			{
				switch(errno)
				{
					case EINTR:
						continue;

					case EFAULT:
					case ENOMEM:
					case ENOBUFS:
						throw FileGenericException("Buffer issue");

					case EIO:
						throw FileIOException("Encountered IO error");

					default:
						throw FileGenericException("Failed to read from file");
				}
			}

			totalRead += bytesRead;
			left -= bytesRead;

		} while(left > 0);

		return totalRead;
	}

	Data *File::ReadData(size_t maxLength)
	{
		uint8 *buffer = (uint8 *)malloc(maxLength);
		size_t read = Read(buffer, maxLength);

		Data *data = new Data(buffer, read, true, true);
		return data->Autorelease();
	}



	size_t File::Write(const void *buffer, size_t size)
	{
		RN_ASSERT(_mode & Mode::Write, "Trying to write to a file not opened for writing");

		size_t totalWritten = 0;
		size_t left = size;
		const uint8 *bytes = static_cast<const uint8 *>(buffer);

		int error = errno;
		ScopeGuard guard([=]{
			errno = error;
		});

		do {

			ssize_t written = write(_fd, bytes + totalWritten, left);
			if(written == 0)
				break;

			if(written == -1)
			{
				switch(errno)
				{
					case EINTR:
						continue;

					case EFAULT:
					case ENOMEM:
						throw FileGenericException("Buffer issue");

					case EIO:
						throw FileIOException("Encountered IO error");

					default:
						throw FileGenericException("Failed to read from file");
				}
			}

			totalWritten += written;
			left -= written;

		} while(left > 0);

		return totalWritten;
	}

	size_t File::WriteData(const Data *data)
	{
		return Write(data->GetBytes(), data->GetLength());
	}



	int File::CreateFileDescriptor() const
	{
		return dup(_fd);
	}

	FILE *File::CreateFile() const
	{
		const char *mode;

		if(_mode & (Mode::Read | Mode::Write))
		{
			if(_mode & Mode::Append)
				mode = "a+b";
			else
				mode = "w+b";
		}
		else if(_mode & Mode::Read)
		{
			mode = "rb";
		}
		else
		{
			if(_mode & Mode::Append)
				mode = "ab";
			else
				mode = "wb";
		}

		return fdopen(dup(_fd), mode);
	}


	int  File::__FileWithPath(const String *name, Mode mode)
	{
		int oflag = O_BINARY;

		if(mode & (Mode::Read | Mode::Write))
			oflag |= O_RDWR;
		else if(mode & Mode::Read)
			oflag |= O_RDONLY;
		else if(mode & Mode::Write)
			oflag |= O_WRONLY;

		if(mode & Mode::Write)
		{
			oflag |= O_APPEND;

			if(!(mode & Mode::NoCreate))
				oflag |= O_CREAT;
		}

		if(!(mode & Mode::Append) && !(mode & Mode::Read))
			oflag |= O_TRUNC;

		// Open the file
		int error = errno;

		int fd = open(name->GetUTF8String(), oflag, 0644);
		if(fd == -1)
		{
			errno = error;
			throw FileNotFoundException("Couldn't find file");
		}

		return fd;
	}

	File *File::WithName(const String *name, Mode mode)
	{
		FileManager *coordinator = FileManager::GetSharedInstance();

		if(mode & Mode::Write)
		{
			bool isDirectory;
			String *basePath = name->StringByDeletingLastPathComponent();

			if(coordinator->PathExists(name) || (coordinator->PathExists(basePath, isDirectory) && isDirectory))
			{
				int fd = __FileWithPath(name, mode);
				File *file = new File(fd, name, mode);

				return file->Autorelease();
			}
		}

		FileManager::ResolveHint hint = 0;

		if(mode & Mode::Write && !(mode & Mode::NoCreate))
			hint |= FileManager::ResolveHint::CreateNode;

		String *path = coordinator->ResolveFullPath(name, hint);
		if(!path)
			throw FileNotFoundException("Couldn't find file");

		int fd = __FileWithPath(path, mode);
		File *file = new File(fd, path, mode);

		return file->Autorelease();
	}


#define ConvenienceReader(type, name) \
	type File::name() \
	{ \
		type buffer; \
		__unused size_t read = Read(&buffer, sizeof(type)); \
		return buffer; \
	}

	ConvenienceReader(uint8, ReadUint8)
	ConvenienceReader(uint16, ReadUint16)
	ConvenienceReader(uint32, ReadUint32)
	ConvenienceReader(uint64, ReadUint64)
	ConvenienceReader(int8, ReadInt8)
	ConvenienceReader(int16, ReadInt16)
	ConvenienceReader(int32, ReadInt32)
	ConvenienceReader(int64, ReadInt64)
	ConvenienceReader(float, ReadFloat)
	ConvenienceReader(double, ReadDouble)
}
