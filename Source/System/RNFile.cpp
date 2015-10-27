//
//  RNFile.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNFile.h"
#include "RNFileCoordinator.h"

namespace RN
{
	RNDefineMeta(File, Object)

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
		uint8 *buffer = new uint8[maxLength];
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



	int  File::__FileWithPath(const String *name, Mode mode)
	{
		int oflag = 0;

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

		if(!(mode & Mode::Append))
			oflag |= O_TRUNC;

		// Open the file
		int error = errno;

		int fd = open(name->GetUTF8String(), oflag);
		if(fd == -1)
		{
			errno = error;
			throw FileNotFoundException("Couldn't find file");
		}

		return fd;
	}

	File *File::WithName(const String *name, Mode mode)
	{
		FileCoordinator *coordinator = FileCoordinator::GetSharedInstance();

		if(mode & Mode::Write)
		{
			bool isDirectory;
			String *basePath = name->StringByDeletingLastPathComponent();

			if(coordinator->PathExists(name) || (coordinator->PathExists(name, isDirectory) && isDirectory))
			{
				int fd = __FileWithPath(name, mode);
				File *file = new File(fd, name, mode);

				return file->Autorelease();
			}
		}

		FileCoordinator::ResolveHint hint = 0;

		if(mode & Mode::Write && !(mode & Mode::NoCreate))
			hint |= FileCoordinator::ResolveHint::CreateNode;

		String *path = coordinator->ResolveFullPath(name, hint);
		if(!path)
			throw FileNotFoundException("Couldn't find file");

		int fd = __FileWithPath(path, mode);
		File *file = new File(fd, path, mode);

		return file->Autorelease();
	}
}
