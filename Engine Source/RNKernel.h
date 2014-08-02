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

#define kRNKernelDidBeginFrameMessage RNCSTR("kRNKernelDidBeginFrameMessage")
#define kRNKernelDidEndFrameMessage   RNCSTR("kRNKernelDidEndFrameMessage")

namespace RN
{
	class WorldCoordinator;
	class KernelInternal;
	class Window;
	class Context;

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
}

#endif /* __RAYNE_KERNEL_H__ */
