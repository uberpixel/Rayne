//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNBaseInternal.h"
#include "RNContextInternal.h"
#include "RNWorld.h"
#include "RNOpenGL.h"
#include "RNThreadPool.h"
#include "RNSettings.h"
#include "RNArray.h"
#include "RNModule.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNCPU.h"
#include "RNResourceCoordinator.h"
#include "RNRenderer32.h"
#include "RNLogging.h"
#include "RNShaderCache.h"
#include "RNWindowInternal.h"
#include "RNWorldCoordinator.h"
#include "RNOpenGLQueue.h"
#include "RNMessage.h"
#include "RNInput.h"
#include "RNUIServer.h"
#include "RNWindow.h"

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: Misc
	// ---------------------

#if RN_PLATFORM_INTEL
	namespace X86_64
	{
		extern void GetCPUInfo();
	}
#endif
	
	namespace Debug
	{
		extern void InstallDebugDraw();
	}
	
	namespace gl
	{
		extern Context *Initialize();
	}
	
	
	static std::atomic<bool> __needsCleanup(false);
	
	void KernelCleanUp()
	{
		if(!__needsCleanup.exchange(false))
			return;
		
		AutoreleasePool pool;
		
		Log::Logger::ResignSharedInstance();
		ShaderCache::ResignSharedInstance();
		Settings::ResignSharedInstance();
	}
	
	// ---------------------
	// MARK: -
	// MARK: KernelInternal
	// ---------------------
	
	class KernelInternal
	{
	public:
		std::vector<Function> _functions;
		std::vector<Timer *>  _timer;
		
		std::string _title;
		FrameID _frame;
		float _scaleFactor;
		
		Application *_app;
		Thread *_mainThread;
		
		Window *_window;
		Context *_context;
		
		AutoreleasePool *_pool;
		Renderer *_renderer;
		Input *_input;
		WorldCoordinator *_worldCoordinator;
		UI::Server *_uiserver;
		
		uint32 _statisticsSwitch;
		Statistics _statistics[2];
		
		uint32 _maxFPS;
		float _minDelta;
		
		bool _fixedDelta;
		bool _resetDelta;
		bool _shouldExit;
		bool _initialized;
		
		float _fixedDeltaTime;
		float _delta;
		double _timeScale;
		double _time;
		double _scaledTime;
		
		std::chrono::steady_clock::time_point _lastFrame;
		
#if RN_PLATFORM_WINDOWS
		HWND _mainWindow;
		HINSTANCE _instance;
		WNDCLASSEXW _windowClass;
#endif
	};
	
	// ---------------------
	// MARK: -
	// MARK: Kernel
	// ---------------------
	
	RNDefineSingleton(Kernel)
	
	Kernel::Kernel(Application *app)
	{
		static std::once_flag flag;
		std::call_once(flag, [] {
			atexit(KernelCleanUp);
		});
		
		Prepare();
		
		_internals->_app = app;
		__needsCleanup.store(true);
	}
	

	Kernel::~Kernel()
	{
		RNDebug("Shutting down...");
		
		__needsCleanup.store(false);
		
		_internals->_worldCoordinator->__AwaitLoadingForExit();
		_internals->_app->WillExit();
		
		ModuleCoordinator::ResignSharedInstance();
		
		delete _internals->_worldCoordinator;
		delete _internals->_renderer;
		delete _internals->_input;
		delete _internals->_uiserver;

#if RN_PLATFORM_LINUX
		Display *dpy = Context::_dpy;
		_context->Release();
		XCloseDisplay(dpy);
#else
		_internals->_context->Release();
#endif

#if RN_PLATFORM_WINDOWS
		::DestroyWindow(_internals->_mainWindow);
#endif

		Settings::ResignSharedInstance();
		ShaderCache::ResignSharedInstance();
		OpenGLQueue::ResignSharedInstance();
		MessageCenter::ResignSharedInstance();
		
		delete _internals->_pool;
		
		_internals->_mainThread->Exit();
		_internals->_mainThread->Release();
		
		ThreadCoordinator::ResignSharedInstance();
		Log::Logger::ResignSharedInstance();
	}
	
	void Kernel::Prepare()
	{
		MakeShared();
		
#if RN_PLATFORM_LINUX
		XInitThreads();
#endif
#if RN_PLATFORM_WINDOWS
		_internals->_instance = (HINSTANCE)::GetModuleHandle(nullptr);

		_internals->_windowClass.cbSize = sizeof(WNDCLASSEXW);
		_internals->_windowClass.style = CS_OWNDC;
		_internals->_windowClass.lpfnWndProc = &WindowProc;
		_internals->_windowClass.cbClsExtra = 0;
		_internals->_windowClass.cbWndExtra = 0;
		_internals->_windowClass.hInstance = _instance;
		_internals->_windowClass.hIcon = LoadIconA(_instance, MAKEINTRESOURCE(1));
		_internals->_windowClass.hCursor = LoadCursorA(nullptr, IDC_ARROW);
		_internals->_windowClass.hbrBackground = nullptr;
		_internals->_windowClass.lpszMenuName = nullptr;
		_internals->_windowClass.lpszClassName = L"RNWindowClass";
		_internals->_windowClass.hIconSm = nullptr;
		
		::RegisterClassExW(&_internals->_windowClass);

		_internals->_mainWindow = ::CreateWindowExW(0, L"RNWindowClass", L"", WS_POPUP | WS_CLIPCHILDREN, 0, 0, 640, 480, nullptr, nullptr, _instance, nullptr);

		::SetFocus(_internals->_mainWindow);
		::SetCursor(_internals->_windowClass.hCursor);

		::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
#endif
		
		OpenGLQueue::GetSharedInstance();
		
		// Bootstrap the very basic things
		_internals->_mainThread = new Thread();
		_internals->_pool = new AutoreleasePool();
		
		_internals->_fixedDelta = false;
		_internals->_statisticsSwitch = 0;
		
		_internals->_title = (Settings::GetSharedInstance()->GetManifestObjectForKey<String>(kRNManifestApplicationKey)->GetUTF8String());
		Settings::GetSharedInstance()->LoadSettings();
		
		ThreadCoordinator::GetSharedInstance();
		Log::Logger::GetSharedInstance();
		
		// Get informations about the hardware
#if RN_PLATFORM_INTEL
		X86_64::GetCPUInfo();
		X86_64::Capabilities caps = X86_64::GetCapabilites();
		
		if(!(caps & X86_64::CAP_SSE) || !(caps & X86_64::CAP_SSE2))
			throw Exception(Exception::Type::NoCPUException, "The CPU doesn't support SSE and/or SSE2!");
#endif
		
		// Dump some hardware info
		DumpSystem();
		
		// Bootstrap OpenGL
		_internals->_context = gl::Initialize();
		_internals->_window  = nullptr;
		
		// Load the shader cache into memory
		ShaderCache::GetSharedInstance()->InitializeDatabase();
		
		_internals->_scaleFactor = 1.0f;
#if RN_PLATFORM_IOS
		_internals->_scaleFactor = [[UIScreen mainScreen] scale];
#endif
#if RN_PLATFORM_MAC_OS
		if([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)])
			_internals->_scaleFactor = [[NSScreen mainScreen] backingScaleFactor];
#endif
		
		if(_internals->_scaleFactor >= 1.5f)
			FileManager::GetSharedInstance()->AddFileModifier("@2x");
		
		Debug::InstallDebugDraw();
		ResourceCoordinator::GetSharedInstance()->LoadEngineResources();
		
		// Bootstrap some more core systems while the resources are loading
		_internals->_renderer = new Renderer32();
		_internals->_input    = Input::GetSharedInstance();
		_internals->_uiserver = UI::Server::GetSharedInstance();
		_internals->_window   = Window::GetSharedInstance();
		_internals->_worldCoordinator = WorldCoordinator::GetSharedInstance();
		
		// Initialize some state
		_internals->_frame  = 0;
		_internals->_maxFPS = 0;
		
		SetMaxFPS(120);
		
		_internals->_delta = 0.0f;
		_internals->_time  = 0.0;
		_internals->_scaledTime = 0.0;
		_internals->_timeScale  = 1.0f;
		
		_internals->_lastFrame  = std::chrono::steady_clock::now();
		_internals->_resetDelta = true;
		
		_internals->_initialized = false;
		_internals->_shouldExit  = false;
		
		_internals->_uiserver->UpdateSize();
		
		// Load all modules
		ModuleCoordinator::GetSharedInstance();
	}	

	void Kernel::DumpSystem()
	{
		Log::Logger *logger = Log::Logger::GetSharedInstance();
		
		std::stringstream cpustream;
		std::stringstream memorystream;
		
		// Rayne version
		{
			std::stringstream version;
			version << GetVersionMajor() << "." << GetVersionMinor() << "." << GetVersionPatch() << ":" << GetVersion();
			
#if RN_PLATFORM_32BIT
			version << " 32 bit";
#endif
#if RN_PLATFORM_64BIT
			version << " 64 bit";
#endif
			version << (IsDebugBuild() ? "(debug)" : "(release)");
			logger->Log(Log::Message(Log::Level::Info, "Rayne", version.str()));
		}
		
		
#if RN_PLATFORM_MAC_OS
		
		size_t size;
		int64  value;
		int    name[2];
		char   string[256];
		
		// OS Version
		{
			NSString *version = [[NSProcessInfo processInfo] operatingSystemVersionString];
			logger->Log(Log::Message(Log::Level::Info, "OS", std::string("Mac OS X ") + [version UTF8String]));
		}
		
		// CPU
		{
			size = 255;
			name[0] = CTL_HW;
			name[1] = HW_MACHINE;
			
			if(sysctl(name, 2, string, &size, nullptr, 0) == 0)
			{
				string[size] = '\0';
				cpustream << string << std::endl;
			}
			
			size = 8;
			if(sysctlbyname("hw.cpufrequency", &value, &size, nullptr, 0) == 0)
			{
				cpustream.precision(3);
				cpustream << std::fixed << (value / 1000000000.0f) << " GHz";
			}
		}
		
		// Memory
		{
			size = 8;
			name[0] = CTL_HW;
			name[1] = HW_MEMSIZE;
			
			if(sysctl(name, 2, &value, &size, nullptr, 0) == 0)
			{
				memorystream << (((value >> 20) + 31) & ~31) << " MB";
			}
		}
		
#endif
		
#if RN_PLATFORM_WINDOWS
		
		// OS Version
		{
			OSVERSIONINFO versionInfo;
			
			versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionExA(&versionInfo);
			
			std::stringstream version;
			version << "Windows " << static_cast<uint32>(versionInfo.dwMajorVersion) << "." << static_cast<uint32>(versionInfo.dwMinorVersion);
			version << versionInfo.szCSDVersion;
			
			logger->Log(Log::Message(Log::Level::Info, "OS", version.str()));
		}
		
		// CPU
		{
			HKEY handle;
			
			if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &handle) == ERROR_SUCCESS)
			{
				DWORD type;
				DWORD size;
				BYTE  data[256];
				
				size = 255;
				if((RegQueryValueExA(handle, "ProcessorNameString", 0, &type, data, &size) == ERROR_SUCCESS) && (type == REG_SZ))
				{
					data[size] = '\0';
					cpustream << reinterpret_cast<char *>(data) << std::endl;
				}
				
				size = 255;
				if((RegQueryValueExA(handle, "Identifier", 0, &type, data, &size) == ERROR_SUCCESS) && (type == REG_SZ))
				{
					data[size] = '\0';
					cpustream << reinterpret_cast<char *>(data) << std::endl;
				}
				
				size = 255;
				if((RegQueryValueExA(handle, "~MHz", 0, &type, data, &size) == ERROR_SUCCESS) && (type == REG_DWORD))
				{
					uint32 mhz = (*reinterpret_cast<uint32 *>(data));
					
					cpustream.precision(3);
					cpustream << std::fixed << (mhz / 1000.0f) << " GHz";
				}
			}
		}
		
		// Memory
		{
			MEMORYSTATUSEX memoryStatus;
			memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
			
			GlobalMemoryStatusEx(&memoryStatus);
			
			memorystream << (((memoryStatus.ullTotalPhys >> 20) + 31) & ~31) << " MB";
		}
#endif
		
		// Additional CPU info
		{
			size_t cores = std::thread::hardware_concurrency();
			
			cpustream << std::endl << cores  << " logical core";
			
			if(cores != 1)
				cpustream << "s";
		
#if RN_PLATFORM_INTEL
			X86_64::Capabilities caps = X86_64::GetCapabilites();
			std::vector<std::string> scaps;
			
			if(caps & X86_64::CAP_SSE)
				scaps.emplace_back("SSE");
			if(caps & X86_64::CAP_SSE2)
				scaps.emplace_back("SSE2");
			if(caps & X86_64::CAP_SSE3)
				scaps.emplace_back("SSE3");
			if(caps & X86_64::CAP_SSE41)
				scaps.emplace_back("SSE4.1");
			if(caps & X86_64::CAP_SSE42)
				scaps.emplace_back("SSE4.2");
			if(caps & X86_64::CAP_AVX)
				scaps.emplace_back("AVX");
			
			cpustream << std::endl;
			bool skipped = false;
			
			for(std::string& cap : scaps)
			{
				if(skipped)
					cpustream << ", ";
				
				cpustream << cap;
				
				if(!skipped)
					skipped = true;
			}
#endif
		}
		
		logger->Log(Log::Message(Log::Level::Info, "CPU", cpustream.str()));
		logger->Log(Log::Message(Log::Level::Info, "Memory", memorystream.str()));
		
		// Modules
		{
			Array *array = Settings::GetSharedInstance()->GetManifestObjectForKey<Array>(KRNManifestModulesKey);
			if(array)
			{
				std::vector<std::string> modules;
				
				array->Enumerate([&](Object *file, size_t index, bool &stop) {
					
					try
					{
						String *string = file->Downcast<String>();
						modules.emplace_back(string->GetUTF8String());
					}
					catch(Exception e)
					{}
					
				});
				
				bool skipped = false;
				std::stringstream stream;
				
				for(std::string& module : modules)
				{
					if(skipped)
						stream << ", ";
					
					stream << module;
					
					if(!skipped)
						skipped = true;
				}
				
				logger->Log(Log::Message(Log::Level::Info, "Modules", stream.str()));
			}
		}
		
		logger->Log(Log::Level::Warning, "");
	}
	
	void Kernel::Initialize()
	{
		_internals->_app->Start();
		_internals->_window->SetTitle(_internals->_app->GetTitle());
	}

	
	
	bool Kernel::Tick()
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _internals->_lastFrame).count();
		double trueDelta  = milliseconds / 1000.0;
		
		if(_internals->_fixedDelta)
			trueDelta = _internals->_fixedDeltaTime;
		
		if(_internals->_resetDelta)
		{
			trueDelta   = 0.0;
			_internals->_resetDelta = false;
		}
		
		if(RN_EXPECT_FALSE(!_internals->_initialized))
		{
			Initialize();
			trueDelta = 0.0;
			
			_internals->_initialized = true;
			_internals->_resetDelta  = true;
			
			RNDebug("First frame");
		}
		
		_internals->_time += trueDelta;

		_internals->_delta = trueDelta * _internals->_timeScale;
		_internals->_scaledTime += _internals->_delta;
		
		_internals->_statisticsSwitch = (++ _internals->_statisticsSwitch) % 2;
		_internals->_statistics[_internals->_statisticsSwitch].Clear();
		
		PushStatistics("krn.events");
		
#if RN_PLATFORM_MAC_OS
		@autoreleasepool
		{
			NSDate *date = [NSDate date];
			NSEvent *event;
			
			while((event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:date inMode:NSDefaultRunLoopMode dequeue:YES]))
			{
				[NSApp sendEvent:event];
				[NSApp updateWindows];
			}
		}
#endif

#if RN_PLATFORM_WINDOWS
		MSG	message;

		while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
#endif
		
#if RN_PLATFORM_LINUX
		XEvent event;
		Atom wmDeleteMessage = XInternAtom(_context->_dpy, "WM_DELETE_WINDOW", False);
		while(XPending(_context->_dpy))
		{
			XNextEvent(_context->_dpy, &event);
			
			switch(event.type)
			{
				case ConfigureNotify:
				case Expose:
				case ClientMessage:
					if(event.xclient.data.l[0] == wmDeleteMessage)
						Exit();
					break;
					
				default:
					_input->HandleEvent(&event);
					break;
			}
		}
#endif
		
		PopStatistics();
		
		_internals->_frame ++;
		_internals->_renderer->BeginFrame(_internals->_delta);
		_internals->_input->DispatchInputEvents();
		
		// Run scheduled functions
		if(!_internals->_functions.empty())
		{
			std::vector<Function> temp;

			{
				LockGuard<SpinLock> lock(_lock);
				std::swap(temp, _internals->_functions);
			}

			for(Function &f : temp)
				f();
		}
	
		// Timer
		if(!_internals->_timer.empty())
		{
			std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
			std::vector<Timer *> temp;
			
			{
				LockGuard<SpinLock> lock(_lock);
				
				for(Timer *timer : _internals->_timer)
				{
					if(timer->GetFireDate() <= time)
						temp.push_back(timer);
				}
			}
			
			for(Timer *timer : temp)
				timer->Fire();
		}
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNKernelDidBeginFrameMessage, nullptr, nullptr);
		
		_internals->_app->GameUpdate(_internals->_delta);

		_internals->_worldCoordinator->StepWorld(_internals->_frame, _internals->_delta);
		_internals->_app->WorldUpdate(_internals->_delta);
		_internals->_worldCoordinator->RenderWorld(_internals->_renderer);
		
		
		_internals->_uiserver->Render(_internals->_renderer);
		
		_internals->_renderer->FinishFrame();
		_internals->_input->InvalidateFrame();
		
		Settings::GetSharedInstance()->Sync(false);
		Log::Logger::GetSharedInstance()->Flush(true);
		MessageCenter::GetSharedInstance()->PostMessage(kRNKernelDidEndFrameMessage, nullptr, nullptr);
		
		PushStatistics("krn.flush");
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			_internals->_window->Flush();
		}, true);
		
		PopStatistics();
		
		_internals->_lastFrame = now;
		_internals->_pool->Drain();
		
		// See how long this frame took and wait if needed
		if(_internals->_maxFPS > 0)
		{
			now = std::chrono::steady_clock::now();
			
			milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _internals->_lastFrame).count();
			trueDelta    = milliseconds / 1000.0f;
			
			if(_internals->_minDelta > trueDelta)
			{
				PushStatistics("krn.sleep");
				
				long sleepTime = (_internals->_minDelta - trueDelta) * 1000000;
				
				if(sleepTime > 1000)
					std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
				
				PopStatistics();
			}
		}
		
		return (_internals->_shouldExit == false);
	}

	void Kernel::Exit()
	{
		_internals->_shouldExit = true;
	}

	
	void Kernel::PushStatistics(const std::string& key)
	{
		_internals->_statistics[_internals->_statisticsSwitch].Push(key);
	}
	
	void Kernel::PopStatistics()
	{
		_internals->_statistics[_internals->_statisticsSwitch].Pop();
	}
	
	
	const std::vector<Statistics::DataPoint *>& Kernel::GetStatisticsData() const
	{
		uint32 index = (_internals->_statisticsSwitch + 1) % 2;
		return _internals->_statistics[index].GetDataPoints();
	}
	
	void Kernel::SetTimeScale(double timeScale)
	{
		LockGuard<SpinLock> lock(_lock);
		_internals->_timeScale = timeScale;
	}
	
	void Kernel::SetFixedDelta(float delta)
	{
		LockGuard<SpinLock> lock(_lock);
		
		_internals->_fixedDeltaTime = delta;
		_internals->_fixedDelta = (delta > 0);
	}
	
	void Kernel::SetMaxFPS(uint32 fps)
	{
		LockGuard<SpinLock> lock(_lock);
		
		_internals->_maxFPS = fps;
		
		if(_internals->_maxFPS > 0)
			_internals->_minDelta = 1.0f / _internals->_maxFPS;
	}
	
	void Kernel::ScheduleFunction(Function &&function)
	{
		LockGuard<SpinLock> lock(_lock);
		_internals->_functions.push_back(std::move(function));
	}
	
	void Kernel::ScheduleTimer(Timer *timer)
	{
		LockGuard<SpinLock> lock(_lock);
		_internals->_timer.push_back(timer->Retain());
	}
	
	void Kernel::RemoveTimer(Timer *timer)
	{
		LockGuard<SpinLock> lock(_lock);
		
		for(auto i = _internals->_timer.begin(); i != _internals->_timer.end();)
		{
			if(*i == timer)
			{
				timer->Release();
				i = _internals->_timer.erase(i);
				continue;
			}
			
			i ++;
		}
	}

	void Kernel::DidSleepForSignificantTime()
	{
		LockGuard<SpinLock> lock(_lock);
		
		_internals->_resetDelta = true;
		_internals->_delta = 0.0f;
	}
	
	float Kernel::GetScaleFactor() const
	{
		return _internals->_scaleFactor;
	}
	
	float Kernel::GetActiveScaleFactor() const
	{
		if(!_internals->_window)
			return _internals->_scaleFactor;
		
		return _internals->_window->GetActiveScreen()->GetScaleFactor();
	}
	
	const std::string &Kernel::GetTitle() const
	{
		return _internals->_title;
	}
	
	uint32 Kernel::GetMaxFPS() const
	{
		return _internals->_maxFPS;
	}
	
	float Kernel::GetDelta() const
	{
		return _internals->_delta;
	}
	
	double Kernel::GetTimeScale() const
	{
		return _internals->_timeScale;
	}
	
	double Kernel::GetTime() const
	{
		return _internals->_time;
	}
	
	double Kernel::GetScaledTime() const
	{
		return _internals->_scaledTime;
	}
	
	FrameID Kernel::GetCurrentFrame() const
	{
		return _internals->_frame;
	}
	
	Context *Kernel::GetContext() const
	{
		return _internals->_context;
	}
}
