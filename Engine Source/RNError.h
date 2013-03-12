//
//  RNError.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ERROR_H__
#define __RAYNE_ERROR_H__

// We can't include the RNBase.h, so we have to include the common headers ourself
#include <string>
#include <vector>

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
		kErrorGroupEngine   = 0x0,
		kErrorGroupGraphics = 0x1,
		kErrorGroupMath     = 0x2,
		kErrorGroupSystem   = 0x3,
		kErrorGroupInput    = 0x4,
		kErrorGroupNetwork  = 0x5
	};
	
	class Thread;
	class ErrorException
	{
	public:
		ErrorException(ErrorCode error, const std::string& description="", const std::string& detail="");
		ErrorException(uint32 group, uint32 subgroup, uint32 code, const std::string& description="", const std::string& detail="");
		
		uint32 Group() const { return RNErrorGetGroup(_error); }
		uint32 Subgroup() const { return RNErrorGetSubroup(_error); }
		uint32 Code() const { return RNErrorGetCode(_error); }
		
		
		ErrorCode Error() const { return _error; }
		Thread *Thread() const { return _thread; }
		
		const std::string& Description() const { return _description; }
		const std::string& AdditionalDetails() const { return _additionalDetail; }
		
		const std::vector<std::pair<uintptr_t, std::string>>& CallStack() const { return _callStack; }
		
	private:
		void GatherInfo();
		
		ErrorCode _error;
		RN::Thread *_thread;
		
		std::string _description;
		std::string _additionalDetail;
		
		std::vector<std::pair<uintptr_t, std::string>> _callStack;
	};
}

#endif /* __RAYNE_ERROR_H__ */
