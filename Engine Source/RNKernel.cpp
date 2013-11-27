//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNBaseInternal.h"
#include "RNContextInternal.h"
#include "RNWorld.h"
#include "RNOpenGL.h"
#include "RNThreadPool.h"
#include "RNSettings.h"
#include "RNModule.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNCPU.h"
#include "RNResourcePool.h"
#include "RNRenderer32.h"
#include "RNLogging.h"
#include "RNShaderCache.h"

#if RN_PLATFORM_IOS
extern "C" RN::Application *RNApplicationCreate(RN::Kernel *);
#endif

typedef RN::Application *(*RNApplicationEntryPointer)(RN::Kernel *);
RNApplicationEntryPointer __ApplicationEntry = 0;

namespace RN
{
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
	
	void KernelCleanUp()
	{
		AutoreleasePool pool;
		
		delete ShaderCache::GetSharedInstance_NoCreate();
		delete Log::Logger::GetSharedInstance_NoCreate();
		delete Settings::GetSharedInstance_NoCreate();
		delete Input::GetSharedInstance_NoCreate();
		delete ModuleCoordinator::GetSharedInstance_NoCreate();
	}
	
	
	
	Kernel::Kernel()
	{
		Prepare();
		LoadApplicationModule(Settings::GetSharedInstance()->GetManifestObjectForKey<String>(kRNManifestGameModuleKey));
	}
	
	Kernel::Kernel(Application *app)
	{
		Prepare();
		_app = app;
	}
	

	Kernel::~Kernel()
	{
		RNDebug("Shutting down...");
		
		_app->WillExit();
		
		delete ModuleCoordinator::GetSharedInstance_NoCreate();
		delete _app;

		delete _renderer;
		delete _input;
		delete _uiserver;

#if RN_PLATFORM_LINUX
		Display *dpy = Context::_dpy;
		_context->Release();
		XCloseDisplay(dpy);
#else
		_context->Release();
#endif

		delete Settings::GetSharedInstance_NoCreate();
		delete _pool;
		
		_mainThread->Exit();
		_mainThread->Release();
		
		delete ShaderCache::GetSharedInstance_NoCreate();
		delete Log::Logger::GetSharedInstance_NoCreate();
	}
	
	void Kernel::Prepare()
	{
#if RN_PLATFORM_LINUX
		XInitThreads();
#endif
		atexit(KernelCleanUp);
		
		// Bootstrap the very basic things
		_mainThread = new Thread();
		_pool = new AutoreleasePool();
		
		_title = (Settings::GetSharedInstance()->GetManifestObjectForKey<String>(kRNManifestApplicationKey)->GetUTF8String());
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
		_context = gl::Initialize();
		_window  = nullptr;
		
		// Load the shader cache into memory
		ShaderCache::GetSharedInstance()->InitializeDatabase();
		
		_scaleFactor = 1.0f;
#if RN_PLATFORM_IOS
		_scaleFactor = [[UIScreen mainScreen] scale];
#endif
#if RN_PLATFORM_MAC_OS
		if([[NSScreen mainScreen] respondsToSelector:@selector(backingScaleFactor)])
			_scaleFactor = [[NSScreen mainScreen] backingScaleFactor];
#endif
		
		// Load the default resources
		_resourceBatch = ThreadPool::GetSharedInstance()->CreateBatch();
		_resourceBatch->AddTask([] {
			Debug::InstallDebugDraw();
		});
		
		ResourcePool::GetSharedInstance()->LoadDefaultResources(_resourceBatch);
		_resourceBatch->Commit();
		
		// Bootstrap some more core systems while the resources are loading
		_renderer = new Renderer32();
		_input    = Input::GetSharedInstance();
		_uiserver = UI::Server::GetSharedInstance();
		_window   = Window::GetSharedInstance();
		
		// Initialize some state
		_world = nullptr;
		_frame  = 0;
		
		_delta = 0.0f;
		_time  = 0.0f;
		_scaledTime = 0.0f;
		_timeScale  = 1.0f;
		
		_lastFrame  = std::chrono::steady_clock::now();
		_resetDelta = true;
		
		_initialized = false;
		_shouldExit  = false;
		
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
			version << VersionMajor() << "." << VersionMinor() << "." << VersionPatch() << ":" << Version();
			
#if RN_PLATFORM_32BIT
			version << " 32 bit";
#endif
#if RN_PLATFORM_64BIT
			version << " 64 bit";
#endif
			
#if DEBUG
			version << " (debug)";
#endif
			
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
				if(!cpustream.tellp() != 0)
					cpustream << std::endl;
				
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
					cpustream << reinterpret_cast<char *>(data);
				}
				
				size = 255;
				if((RegQueryValueExA(handle, "Identifier", 0, &type, data, &size) == ERROR_SUCCESS) && (type == REG_SZ))
				{
					if(!cpustream.tellp() != 0)
						cpustream << std::endl;
					
					data[size] = '\0';
					cpustream << reinterpret_cast<char *>(data);
				}
				
				size = 255;
				if((RegQueryValueExA(handle, "~MHz", 0, &type, data, &size) == ERROR_SUCCESS) && (type == REG_DWORD))
				{
					if(!cpustream.tellp() != 0)
						cpustream << std::endl;
					
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
				
				array->Enumerate([&](Object *file, size_t index, bool *stop) {
					
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
	
	void Kernel::LoadApplicationModule(String *module)
	{
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
		std::string moduleName = std::string(module->GetUTF8String());
		
#if RN_PLATFORM_MAC_OS
		moduleName += ".dylib";
#endif
		
#if RN_PLATFORM_LINUX
		moduleName += ".so";
#endif
		
		std::string path = FileManager::GetSharedInstance()->GetFilePathWithName(moduleName);

		_appHandle = dlopen(path.c_str(), RTLD_LAZY);
		if(!_appHandle)
			throw Exception(Exception::Type::ApplicationNotFoundException, std::string(dlerror()));
		
		__ApplicationEntry = (RNApplicationEntryPointer)dlsym(_appHandle, "RNApplicationCreate");

		RN_ASSERT(__ApplicationEntry, "The game module must provide an application entry point (RNApplicationCreate())");
#endif
#if RN_PLATFORM_IOS
		__ApplicationEntry = RNApplicationCreate;
#endif

		_app = __ApplicationEntry(this);
		RN_ASSERT(_app, "The game module must respond to RNApplicationCreate() and return a valid RN::Application object!");
	}

	void Kernel::Initialize()
	{
		_resourceBatch->Wait();
		_resourceBatch->Release();
		_resourceBatch = nullptr;
		
		_app->Start();
		_window->SetTitle(_app->Title());
	}

	
	
	bool Kernel::Tick()
	{
		std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		float trueDelta = milliseconds / 1000.0f;
		
		if(_resetDelta)
		{
			trueDelta = 0.0f;
			_resetDelta = false;
		}
		
		if(!_initialized)
		{
			Initialize();
			trueDelta = 0.0f;
			
			_initialized = true;
			_resetDelta  = true;
			
			RNDebug("First frame");
		}

		_time += trueDelta;

		_delta = trueDelta * _timeScale;
		_scaledTime += _delta;
		
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
		
		_frame ++;
		_renderer->BeginFrame(_delta);
		_input->DispatchInputEvents();
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNKernelDidBeginFrameMessage, nullptr, nullptr);
		
		_app->GameUpdate(_delta);

		if(_world)
		{
			_world->StepWorld(_frame, _delta);
			_app->WorldUpdate(_delta);
			
			_world->RenderWorld(_renderer);
		}
		
		_uiserver->Render(_renderer);
		_renderer->FinishFrame();
		_input->InvalidateFrame();
		
		Settings::GetSharedInstance()->Sync(false);
		Log::Logger::GetSharedInstance()->Flush();
		MessageCenter::GetSharedInstance()->PostMessage(kRNKernelDidEndFrameMessage, nullptr, nullptr);
		
#if RN_PLATFORM_MAC_OS
		CGLFlushDrawable(_context->_internals->cglContext);
#endif
		
#if RN_PLATFORM_LINUX
		glXSwapBuffers(_context->_dpy, _context->_win);
#endif
		_lastFrame = now;
		_pool->Drain();
		
		return (_shouldExit == false);
	}

	void Kernel::Exit()
	{
		_shouldExit = true;
	}


	void Kernel::SetWorld(World *world)
	{
		_world = world;
	}

	void Kernel::SetTimeScale(float timeScale)
	{
		_timeScale = timeScale;
	}

	void Kernel::DidSleepForSignificantTime()
	{
		_resetDelta = true;
		_delta = 0.0f;
	}
	
	float Kernel::GetActiveScaleFactor() const
	{
		if(!_window)
			return _scaleFactor;
		
		return _window->GetActiveScreen()->GetScaleFactor();
	}
}
