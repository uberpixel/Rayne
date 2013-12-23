//
//  RNFile.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_FILE_H__
#define __RAYNE_FILE_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class File : public Object
	{
	public:
		typedef enum
		{
			Read = 0,
			Write = 1,
			ReadWrite = 2
		} FileMode;

		RNAPI File(const std::string& path, FileMode mode = Read);
		RNAPI ~File() override;
		
		// Reading operations
		RNAPI std::string GetString();
		RNAPI std::vector<uint8> GetBytes();
		
		RNAPI void ReadIntoBuffer(void *buffer, size_t size);
		RNAPI void ReadIntoString(std::string& string, size_t size, bool appendNull=true);
		RNAPI void Seek(size_t offset, bool fromStart = false);
		
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

		// Writing operations
		RNAPI void WriteString(const std::string& string, bool includeNull=false);
		RNAPI void WriteBuffer(const void *buffer, size_t size);
		
		RNAPI void WriteUint8(uint8 value);
		RNAPI void WriteUint16(uint16 value);
		RNAPI void WriteUint32(uint32 value);
		RNAPI void WriteUint64(uint64 value);
		
		RNAPI void WriteInt8(int8 value);
		RNAPI void WriteInt16(int16 value);
		RNAPI void WriteInt32(int32 value);
		RNAPI void WriteInt64(int64 value);
		
		RNAPI void WriteFloat(float value);
		RNAPI void WriteDouble(double value);
		
		// Misc
		FILE *GetFilePointer() { return _file; }
		size_t GetSize() const { return static_cast<size_t>(_size); }

		const std::string& GetPath() { return _path; }
		const std::string& GetName() { return _name; }
		const std::string& GetExtension() { return _extension; }
		const std::string& GetFullPath() { return _fullPath; }

	private:
		bool OpenPath(const std::string& path, FileMode mode);

		FileMode _mode;

		std::string _path;
		std::string _name;
		std::string _extension;
		std::string _fullPath;

		FILE *_file;
		long _size;
		
		RNDefineMeta(File, Object)
	};
}

#endif /* __RAYNE_FILE_H__ */
