//
//  RNFile.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FILE_H_
#define __RAYNE_FILE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNData.h"

namespace RN
{
	class File : public Object
	{
	public:
		RN_OPTIONS(Mode, uint32,
				   Read = (1 << 0),
				   Write = (1 << 1),
				   Append = (1 << 2),
				   NoCreate = (1 << 3));

		RNAPI static File *WithName(const String *name, Mode mode = Mode::Read);

		RNAPI ~File();

		size_t GetSize() const { return _size; }
		RNAPI size_t GetOffset() const;

		RNAPI void Seek(size_t offset, bool fromStart = true);

		// Reading
		RNAPI size_t Read(void *buffer, size_t size);
		RNAPI Data *ReadData(size_t maxLength);

		RNAPI uint8 ReadUint8();
		RNAPI uint16 ReadUint16();
		RNAPI uint32 ReadUint32();
		RNAPI uint64 ReadUint64();

		RNAPI int8 ReadInt8();
		RNAPI int16 ReadInt16();
		RNAPI int32 ReadInt32();
		RNAPI int64 ReadInt64();

		RNAPI float ReadFloat();
		RNAPI double ReadDouble();

		// Writing
		RNAPI size_t Write(const void *buffer, size_t size);
		RNAPI size_t WriteData(const Data *data);

		const String *GetPath() const { return _path; }

		// Must be closed with close() and fclose() respectively
		int CreateFileDescriptor() const;
		FILE *CreateFile() const;

	private:
		static int __FileWithPath(const String *name, Mode mode);
		File(int fd, const String *path, Mode mode);

		int _fd;
		size_t _size;
		Mode _mode;
		String *_path;

		RNDeclareMeta(File)
	};

	RNExceptionType(FileNotFound)
	RNExceptionType(FileIO)
	RNExceptionType(FileGeneric)
}


#endif /* __RAYNE_FILE_H_ */
