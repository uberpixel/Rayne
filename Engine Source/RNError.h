//
//  RNError.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ERROR_H__
#define __RAYNE_ERROR_H__

#include <string>

#include "RNPlatform.h"
#include "RNDefines.h"

#include "RNErrorGroupGraphics.h"

#define RNErrorGroup(x)    (((x) & 0x3f) << 26)
#define RNErrorSubgroup(x) (((x) & 0xfff) << 14)

#define RNErrorGetGroup(err)   (((err) >> 26) & 0x3f)
#define RNErrorGetSubroup(err) (((err) >> 14) & 0xfff)
#define RNErrorGetCode(err)    ((err) & 0x3fff)

#define RNErrorGroupMask    (RNErrorGroup(0x3f))
#define RNErrorSubgroupMask (RNErrorSubgroup(0xfff))
#define RNErrorCodeMask     (0x3fff)

namespace RN
{
	typedef uint32 ErrorCode;
	
	enum
	{
		kErrorOkay = 0
	};
	
	
	enum
	{
		kErrorGroupEngine   = RNErrorGroup(0x0),
		kErrorGroupGraphics = RNErrorGroup(0x1),
		kErrorGroupMath     = RNErrorGroup(0x2),
		kErrorGroupSystem   = RNErrorGroup(0x3),
		kErrorGroupInput    = RNErrorGroup(0x4),
		kErrorGroupNetwork  = RNErrorGroup(0x5)
	};
	
	
	class ErrorException
	{
	public:
		ErrorException(ErrorCode error, const std::string& description="") { _error = error; _description = description; }
		ErrorException(uint32 group, uint32 subgroup, uint32 code, const std::string& description="") { _error = group | subgroup | code; _description = description; }
		
		uint32 Group() const { return RNErrorGetGroup(_error); }
		uint32 Subgroup() const { return RNErrorGetSubroup(_error); }
		uint32 Code() const { return RNErrorGetCode(_error); }
		
		ErrorCode Error() const { return _error; }
		const std::string& Description() const { return _description; }
		
	private:
		ErrorCode _error;
		std::string _description;
	};
}

#endif /* __RAYNE_ERROR_H__ */
