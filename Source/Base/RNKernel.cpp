//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "../Debug/RNLoggingEngine.h"
#include "../Objects/RNAutoreleasePool.h"
#include "../Objects/RNJSONSerialization.h"
#include "../Rendering/RNRendererDescriptor.h"
#include "RNBaseInternal.h"
#include "RNPerformanceScenarioRunner.h"
#include "RNScopeAllocator.h"

namespace RN
{
	static Kernel *__sharedInstance = nullptr;

#if RN_ENABLE_VTUNE
	static __itt_string_handle *__inputTask;
	static __itt_string_handle *__updateTask;
	static __itt_string_handle *__renderingTask;

	#define START_TASK(task) \
		__itt_task_begin(VTuneDomain, __itt_null, __itt_null, (task))
	#define END_TASK() \
		__itt_task_end(VTuneDomain)
#else

	#define START_TASK(task) (void)(0)
	#define END_TASK() (void)(0)

#endif

	#if RN_PLATFORM_ANDROID
	Kernel::Kernel(Application *application, const ArgumentParser &arguments, AndroidState *androidState) :
		_arguments(arguments),
		_application(application),
		_androidState(androidState),
		_exit(false),
		_isActive(true)
	{}
#else
	Kernel::Kernel(Application *application, const ArgumentParser &arguments) :
		_arguments(arguments),
		_application(application),
		_exit(false),
		_isActive(true)
	{}
#endif

	Kernel::~Kernel()
	{}

	Kernel *Kernel::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void Kernel::Bootstrap()
	{
		RN_PROFILE_SCOPE();
		__sharedInstance = this;

#if RN_ENABLE_VTUNE
		__inputTask = __itt_string_handle_createA("Input");
		__updateTask = __itt_string_handle_createA("Scene update");
		__renderingTask = __itt_string_handle_createA("Rendering");
#endif

		try
		{
			_observer = new RunLoopObserver(RunLoopObserver::Activity::Finalize, true,
											std::bind(&Kernel::HandleObserver, this, std::placeholders::_1,
													  std::placeholders::_2));
			_mainThread = new Thread();

			RN_UNUSED ScopeAllocator rootAllocator(BumpAllocator::GetThreadAllocator());


			_runLoop = _mainThread->GetRunLoop();
			_runLoop->AddObserver(_observer);

#if RN_PLATFORM_LINUX
			_connection = xcb_connect(nullptr, nullptr);
			if(xcb_connection_has_error(_connection))
			{
				xcb_disconnect(_connection);
				_connection = nullptr;
			}
#endif

			AutoreleasePool pool; // Wrap everyting into an autorelease pool from now on

			WorkQueue::InitializeQueues();

			_fileManager = new FileManager();
			ReadManifest();
			_application->__PrepareForWillFinishLaunching(this); //This takes the game title from the manifest, which is needed to log into the correct directory
			_fileManager->__PrepareWithManifest();

			_mainQueue = WorkQueue::GetMainQueue();
			_logger = new Logger();

			Array *loggers = _application->GetLoggingEngines();
			loggers->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
				_logger->AddEngine(engine);
			});

			Screen::InitializeScreens();
			__ExtensionPointBase::InitializeExtensionPoints();
			_firstFrame = true;
			_frames = 0;

			_delta = 0;
			_time = 0;

			SetMaxFPS(500);

			_settings = new Settings(); // Requires the FileManager to have all search paths
			_performanceScenarioRunner = new PerformanceScenarioRunner();
			_performanceScenarioRunner->LoadDefaultScenario();

			_notificationManager = new NotificationManager();
			_assetManager = AssetManager::GetSharedInstance(); //Potentially needed for registering modules during static initialisation, so GetSharedInstance constructs it if needed
			_assetManager->SetDefaultQueue(WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default));
			_sceneManager = new SceneManager();
			_inputManager = new InputManager();
			_moduleManager = new ModuleManager();

			if(_manifest)
			{
				String *preferredTextureFileExtension = GetManifestEntryForKey<String>(kRNManifestPreferredTextureFileExtensionKey);
				if(preferredTextureFileExtension) _assetManager->SetPreferredTextureFileExtension(preferredTextureFileExtension);
			}

			_moduleManager->LoadModules();
			Catalogue::GetSharedInstance()->RegisterPendingClasses();

			_application->WillFinishLaunching(this);
			_renderer = nullptr;

			if(!_arguments.HasArgument("headless", 'h'))
			{
				RendererDescriptor *descriptor = _application->GetPreferredRenderer();

				if(!descriptor)
				{
					descriptor = RendererDescriptor::GetPreferredRenderer();

					if(!descriptor)
					{
						Array *renderers = RendererDescriptor::GetAvailableRenderers();

						if(renderers->GetCount() > 0)
							descriptor = renderers->GetObjectAtIndex<RendererDescriptor>(0);
					}
				}

				if(descriptor)
				{
					try
					{
						_renderer = RendererDescriptor::ActivateRenderer(descriptor);
					}
					catch(Exception &e)
					{
						_renderer = nullptr;
						RNError("Creating renderer failed with exception: " << e);
					}
				}


				if(!_renderer)
				{
					RNWarning("Non headless rendering requested, but no available rendering descriptor found!");
				}
			}
			else
			{
				RNDebug("Running with headless renderer");
			}
		}
		catch(std::exception &e)
		{
			__sharedInstance = nullptr;

			std::cerr << e.what() << std::endl;
			throw e;
		}
	}

	void Kernel::ReadManifest()
	{
		String *path = _fileManager->GetPathForLocation(FileManager::Location::RootResourcesDirectory);
		path = path->StringByAppendingPathComponent(RNCSTR("manifest.json"));

		Data *data = Data::WithContentsOfFile(path);
		if(!data)
			throw InvalidArgumentException(String::WithFormat("Could not open manifest at path %s", path->GetUTF8String()));

		_manifest = JSONSerialization::ObjectFromData<Dictionary>(data, 0);

		if(!_manifest)
			throw InconsistencyException("Malformed manifest.json");

		String *title = GetManifestEntryForKey<String>(kRNManifestApplicationKey);
		if(!title)
			throw InconsistencyException("Malformed manifest.json, RNApplication key not set");
	}

	Object *Kernel::__GetManifestEntryForKey(const String *key) const
	{
		Object *result;

#if RN_PLATFORM_MAC_OS
		if((result = _manifest->GetObjectForKey(key->StringByAppendingString(RNCSTR("~macos")))))
			return result;
#endif
#if RN_PLATFORM_WINDOWS
		if((result = _manifest->GetObjectForKey(key->StringByAppendingString(RNCSTR("~windows")))))
			return result;
#endif
#if RN_PLATFORM_LINUX
		if((result = _manifest->GetObjectForKey(key->StringByAppendingString(RNCSTR("~linux")))))
			return result;
#endif
#if RN_PLATFORM_ANDROID
		if((result = _manifest->GetObjectForKey(key->StringByAppendingString(RNCSTR("~android")))))
			return result;
#endif

		return _manifest->GetObjectForKey(key);
	}

	void Kernel::FinishBootstrap()
	{
		RN_UNUSED ScopeAllocator rootAllocator(BumpAllocator::GetThreadAllocator());
		_application->DidFinishLaunching(this);
	}

	void Kernel::TearDown()
	{
		RN_UNUSED ScopeAllocator rootAllocator(BumpAllocator::GetThreadAllocator());
		AutoreleasePool pool;

		RNInfo("RN_SHUTDOWN Kernel::TearDown begin");
		_sceneManager->PrepareForShutdown();
		_application->WillExit();
		delete _application;
		_application = nullptr;

		delete _sceneManager;
		_sceneManager = nullptr;
		pool.Drain();

		Screen::TeardownScreens();
		_assetManager->ReleaseKeepAliveAssets();
		Initializer::RunDestructors();
		pool.Drain();

		if(_renderer)
		{
			_renderer->Deactivate();
			_renderer = nullptr;
		}
		pool.Drain();

		_assetManager->CancelPendingLoads();
		WorkQueue::TearDownQueues();
		pool.Drain();

		delete _assetManager;
		_assetManager = nullptr;
		pool.Drain();
#if RN_PLATFORM_LINUX
		if(_connection) xcb_disconnect(_connection);
#endif

		delete _fileManager;
		delete _inputManager;
		delete _moduleManager;
		delete _notificationManager;
		delete _performanceScenarioRunner;

		RNInfo("RN_SHUTDOWN Kernel engine cleanup finished");
		_logger->Flush();
		delete _logger;

		__ExtensionPointBase::TeardownExtensionPoints();
		pool.Drain();

		delete this;
		__sharedInstance = nullptr;
	}

#if RN_PLATFORM_MAC_OS
	void Kernel::__WillBecomeActive()
	{
		_application->WillBecomeActive();
	}
	void Kernel::__DidBecomeActive()
	{
		_isActive = true;
		_application->DidBecomeActive();
	}
	void Kernel::__WillResignActive()
	{
		_application->WillResignActive();
	}
	void Kernel::__DidResignActive()
	{
		_isActive = false;
		_application->DidResignActive();
	}
#endif

#if RN_PLATFORM_ANDROID
	void Kernel::InitializeAndroid(JNIEnv *jniEnv)
	{
		_androidState->SetRayneMainThreadJNIEnv(jniEnv);
	}

	void Kernel::DrainAndroidStateChanges()
	{
		_androidState->DrainPendingState([&](bool windowChanged, bool didStart, bool didResume, bool didPause, bool didStop, bool didDestroy, bool didLowMemory) {
			if(windowChanged)
			{
				RNInfo("Android lifecycle: window changed");
				NotificationManager::GetSharedInstance()->PostNotification(kRNAndroidWindowDidChange, nullptr);
			}

			if(didStart)
			{
				RNInfo("Android lifecycle: started");
			}

			if(didResume)
			{
				RNInfo("Android lifecycle: resumed");
				NotificationManager::GetSharedInstance()->PostNotification(kRNAndroidOnResume, nullptr);
			}

			if(didPause)
			{
				RNInfo("Android lifecycle: paused");
			}

			if(didStop)
			{
				RNInfo("Android lifecycle: stopped");
			}

			if(didDestroy)
			{
				RNInfo("Android lifecycle: destroy requested");
				NotificationManager::GetSharedInstance()->PostNotification(kRNAndroidOnDestroy, nullptr);
			}

			if(didLowMemory)
			{
				RNWarning("Android lifecycle: low memory");
				NotificationManager::GetSharedInstance()->PostNotification(kRNAndroidOnLowMemory, nullptr);
			}
		});
	}
#endif

#if RN_PLATFORM_IOS
	void Kernel::SetMetalLayer(void *metalLayer)
	{
		_metalLayer = metalLayer;
	}
#endif

#if RN_PLATFORM_VISIONOS
	void Kernel::SetLayerRenderer(void *layerRenderer)
	{
		_layerRenderer = layerRenderer;
	}
#endif

	void Kernel::SetMaxFPS(uint32 maxFPS)
	{
		_maxFPS = maxFPS;
		if(maxFPS)
		{
			_minDelta = 1.0 / maxFPS;
		}
		else
		{
			_minDelta = 0.0;
		}
	}

	void Kernel::HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity)
	{
		RN_PROFILE_SCOPE();
		RN_UNUSED ScopeAllocator rootAllocator(BumpAllocator::GetThreadAllocator());

		if(RN_EXPECT_FALSE(_exit))
		{
			_runLoop->Stop();
			return;
		}
#if RN_PLATFORM_MAC_OS
		NSAutoreleasePool *nsautoreleasePool = [[NSAutoreleasePool alloc] init];
#endif

#if RN_ENABLE_VTUNE
		__itt_frame_begin_v3(VTuneDomain, nullptr);
#endif
		auto finishFrame = [&] {
#if RN_PLATFORM_MAC_OS
			[nsautoreleasePool release];
#endif

			// Make sure the run loop wakes up again afterwards
			_runLoop->WakeUp();

#if RN_ENABLE_VTUNE
			__itt_frame_end_v3(VTuneDomain, nullptr);
#endif
		};
		auto finishIfExiting = [&] {
			if(!RN_EXPECT_FALSE(_exit)) return false;

			_runLoop->Stop();
			finishFrame();
			return true;
		};

		AutoreleasePool pool;

		Clock::time_point now = Clock::now();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		_delta = milliseconds / 1000.0;

		if(RN_EXPECT_FALSE(_firstFrame))
		{
			HandleSystemEvents();

			_delta = 0.0;

			HandleSystemEvents();
			FinishBootstrap();

			_firstFrame = false;
		}

		_time.store(_time.load(std::memory_order_relaxed) + _delta, std::memory_order_relaxed);

#if RN_PLATFORM_ANDROID
		HandleSystemEvents();
		if(finishIfExiting()) return;
#endif

		_application->WillStep(static_cast<float>(_delta));
		if(finishIfExiting()) return;

		// Perform work submitted to the main queue
		{
			volatile bool finishWork;
			_mainQueue->Perform([&] {
				finishWork = true;
				_settings->Sync();
			});

			do {
				finishWork = false;
				_mainQueue->PerformWork();
			} while(!finishWork);
		}

		START_TASK(__inputTask);
		// System event handling
		HandleSystemEvents();
		const bool exitRequested = _exit;

		// Update input and then run scene updates
		if(!exitRequested && _isActive)
			_inputManager->Update(static_cast<float>(_delta));
		END_TASK();
		if(finishIfExiting()) return;

		START_TASK(__updateTask);
		_sceneManager->Update(static_cast<float>(_delta));
		END_TASK();

		_application->DidUpdate(static_cast<float>(_delta));
		_performanceScenarioRunner->Update(static_cast<float>(_delta));

		if(_renderer)
		{
			_renderer->Render([&] {
				START_TASK(__renderingTask);
				_sceneManager->Render(_renderer);
				END_TASK();
			});
		}

		_application->DidStep(static_cast<float>(_delta));
		_lastFrame = now;

		// FPS cap
		if(_maxFPS > 0)
		{
			now = Clock::now();

			milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
			double delta = milliseconds / 1000.0;

			if(_minDelta > delta)
			{
				uint32 sleepTime = static_cast<uint32>((_minDelta - delta) * 1000000);
				if(sleepTime > 1000)
					std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
			}
		}

		finishFrame();
	}

	void Kernel::HandleSystemEvents()
	{
		RN_PROFILE_SCOPE();
#if RN_PLATFORM_MAC_OS
		@autoreleasepool
		{
			NSDate *date = [NSDate date];
			NSEvent *event;

			while((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:date inMode:NSDefaultRunLoopMode dequeue:YES]))
			{
				[NSApp sendEvent:event];
				[NSApp updateWindows];
			}
		}
#endif
#if RN_PLATFORM_WINDOWS
		MSG message;
		while(PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
		{
			if(message.message == WM_INPUT)
			{
				InputManager::GetSharedInstance()->__HandleRawInput((HRAWINPUT)message.lParam);
			}

			if(message.message == WM_CLOSE || message.message == WM_DESTROY || message.message == WM_QUIT)
			{
				Exit();
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}
#endif
#if RN_PLATFORM_LINUX
		if(!_connection) return;

		xcb_flush(_connection);

		xcb_generic_event_t *event;
		while((event = xcb_poll_for_event(_connection)))
		{
			switch(event->response_type & ~0x80)
			{
				default:
					break;
			}

			free(event);
		}
#endif
#if RN_PLATFORM_ANDROID
		DrainAndroidStateChanges();
		if(GetAndroidState()->GetDestroyRequested())
		{
			RNInfo("RN_SHUTDOWN Android destroy requested");
			Exit();
		}
#endif
	}

	void Kernel::Run()
	{
		RN_PROFILE_SCOPE();
		_exit = false;

		do {
			_runLoop->Run();
		} while(!_exit);
	}

	void Kernel::Exit()
	{
		RN_PROFILE_SCOPE();
		_exit = true;
	}
} // namespace RN
