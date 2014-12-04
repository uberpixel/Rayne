//
//  RNKernel.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "RNBase.h"
#include "RNAutoreleasePool.h"
#include "RNObject.h"
#include "RNApplication.h"
#include "RNFunction.h"
#include "RNTimer.h"
#include "RNStatistics.h"
#include "RNFileManager.h"
//#include "RNScriptEngine.h"

#define kRNKernelDidBeginFrameMessage RNCSTR("kRNKernelDidBeginFrameMessage")
#define kRNKernelDidEndFrameMessage   RNCSTR("kRNKernelDidEndFrameMessage")

namespace RN
{
	class WorldCoordinator;
	class KernelInternal;
	class Window;
	class Context;
	class ScriptEngine;

	namespace UI
	{
		class Server;
	}
	
	class Kernel : public INonConstructingSingleton<Kernel>
	{
	public:
		friend class Settings;
		friend class Window;
		friend class UI::Server;

		RNAPI Kernel(Application *app);
		RNAPI ~Kernel() override;

		RNAPI bool Tick();
		
		RNAPI void SetFixedDelta(float delta);
		RNAPI void SetTimeScale(double timeScale);
		RNAPI void SetMaxFPS(uint32 fps);
		
		RNAPI void DidSleepForSignificantTime();
		RNAPI void Exit();
		
		RNAPI void ScheduleFunction(Function &&function);
		RNAPI void ScheduleTimer(Timer *timer);
		RNAPI void RemoveTimer(Timer *timer);
		
		RNAPI void PushStatistics(const std::string &key);
		RNAPI void PopStatistics();
		
		RNAPI void AddScriptEngine(ScriptEngine *engine);
		RNAPI void RemoveScriptEngine(String *identifier);
		RNAPI ScriptEngine *GetScriptEngine(String *identifier);
		
		RNAPI const std::vector<Statistics::DataPoint *>& GetStatisticsData() const;

		RNAPI float GetScaleFactor() const;
		RNAPI float GetActiveScaleFactor() const;
		
		RNAPI const std::string &GetTitle() const;
		RNAPI uint32 GetMaxFPS() const;
		
		RNAPI float GetDelta() const;
		RNAPI double GetTimeScale() const;
		RNAPI double GetTime() const;
		RNAPI double GetScaledTime() const;
		
		RNAPI FrameID GetCurrentFrame() const;

#if RN_PLATFORM_WINDOWS
		RNAPI HWND GetMainWindow() const;
		RNAPI HINSTANCE GetInstance() const;
#endif
		
		// Private
		void __WillBecomeActive();
		void __WillResignActive();
		void __DidBecomeActive();
		void __DidResignActive();

	private:
		Context *GetContext() const;
		
		void Prepare();
		void Initialize();
		void DumpSystem();

#if RN_PLATFORM_WINDOWS
		void UseAccelerator(HACCEL accelerator);
#endif

		SpinLock _lock;
		PIMPL<KernelInternal> _internals;
		
		RNDeclareSingleton(Kernel)
	};
	
	template<class T>
	int Main(int argc, char *argv[])
	{
		Initialize(argc, argv);
		
#if RN_PLATFORM_MAC_OS
		FileManager::GetSharedInstance()->AddSearchPath("/usr/local/opt/Rayne/Engine Resources");
#endif
#if RN_PLATFORM_WINDOWS
		char path[MAX_PATH + 1];
		::SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, path);
		
		std::stringstream stream;
		stream << path << "\\Rayne\\Engine Resources";
		
		FileManager::GetSharedInstance()->AddSearchPath(stream.str());
#endif
		
		try
		{
			auto application = new T();
			auto kernel = new Kernel(application);
			
			while(kernel->Tick())
			{}
			
			delete kernel;
			delete application;
		}
		catch(Exception e)
		{
			HandleException(e);
			return EXIT_FAILURE;
		}
		
		return EXIT_SUCCESS;
	}
}

#endif /* __RAYNE_KERNEL_H__ */
