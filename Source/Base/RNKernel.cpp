//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"
#include "RNBaseInternal.h"
#include "../Objects/RNAutoreleasePool.h"
#include "../Objects/RNJSONSerialization.h"
#include "../Rendering/RNRendererDescriptor.h"
#include "../Debug/RNLoggingEngine.h"

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

	Kernel::Kernel(Application *application, const ArgumentParser &arguments) :
		_arguments(arguments),
		_application(application),
		_exit(false),
		_isActive(true)
	{}

	Kernel::~Kernel()
	{}

	Kernel *Kernel::GetSharedInstance()
	{
		return __sharedInstance;
	}


	void Kernel::Bootstrap()
	{
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


			_runLoop = _mainThread->GetRunLoop();
			_runLoop->AddObserver(_observer);

#if RN_PLATFORM_LINUX
			_connection = xcb_connect(nullptr, nullptr);
#endif

			AutoreleasePool pool; // Wrap everyting into an autorelease pool from now on

			WorkQueue::InitializeQueues();

			_mainQueue = WorkQueue::GetMainQueue();
			_logger = new Logger();

			Array *loggers = _application->GetLoggingEngines();
			loggers->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
				_logger->AddEngine(engine);
			});

			Screen::InitializeScreens();
			__ExtensionPointBase::InitializeExtensionPoints();

			_fileManager = new FileManager();
			_firstFrame = true;
			_frames = 0;

			_delta = 0;
			_time = 0;

			ReadManifest();

			_application->__PrepareForWillFinishLaunching(this);
			_fileManager->__PrepareWithManifest();

			_settings = new Settings(); // Requires the FileManager to have all search paths

			_notificationManager = new NotificationManager();
			_assetManager = new AssetManager();
			_sceneManager = new SceneManager();
			_inputManager = new InputManager();
			_moduleManager = new ModuleManager();

			_moduleManager->LoadModules();
			Catalogue::GetSharedInstance()->RegisterPendingClasses();

			_application->WillFinishLaunching(this);
			_renderer = nullptr;

			if(!_arguments.HasArgument("--headless", 'h'))
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
		if((result = _manifest->GetObjectForKey(key->StringByAppendingString(RNCSTR("~osx")))))
			return result;
#endif
#if RN_PLATFORM_WINDOWS
		if((result = _manifest->GetObjectForKey(key->StringByAppendingString(RNCSTR("~win")))))
			return result;
#endif

		return _manifest->GetObjectForKey(key);
	}

	void Kernel::FinishBootstrap()
	{
		_application->DidFinishLaunching(this);
	}

	void Kernel::TearDown()
	{
		_application->WillExit();

		Screen::TeardownScreens();
		WorkQueue::TearDownQueues();

#if RN_PLATFORM_LINUX
		xcb_disconnect(_connection);
#endif

		if(_renderer)
		{
			_renderer->Deactivate();
			delete _renderer;
		}

		delete _fileManager;
		delete _assetManager;
		delete _sceneManager;
		delete _inputManager;
		delete _moduleManager;
		delete _notificationManager;

		_logger->Flush();
		delete _logger;

		__ExtensionPointBase::TeardownExtensionPoints();

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

	void Kernel::HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity)
	{
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

		AutoreleasePool pool;

		Clock::time_point now = Clock::now();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		_delta = milliseconds / 1000.0;


		if(RN_EXPECT_FALSE(_firstFrame))
		{
			HandleSystemEvents();

			if(_renderer)
			{
				Window *window = _renderer->GetMainWindow();
				if(!window)
				{
					//TODO: Take resolution and swap chain descriptor from settings file!
					Window::SwapChainDescriptor swapChainDescriptor(Texture::Format::BGRA8888SRGB);
					window = _renderer->CreateAWindow(Vector2(960, 540), Screen::GetMainScreen(), swapChainDescriptor);
					window->SetTitle(_application->GetTitle());
					window->Show();
					window->Autorelease();
				}
			}

			_delta = 0.0;

			HandleSystemEvents();
			FinishBootstrap();
			
			_firstFrame = false;
		}

		_time += _delta;

		_application->WillStep(static_cast<float>(_delta));

		// Perform work submitted to the main queue
		{
			volatile bool finishWork;
			_mainQueue->Perform([&]{
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

		// Update input and then run scene updates
		if(_isActive)
			_inputManager->Update(static_cast<float>(_delta));
		END_TASK();

		START_TASK(__updateTask);
		_sceneManager->Update(static_cast<float>(_delta));
		END_TASK();

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

#if RN_PLATFORM_MAC_OS
		[nsautoreleasePool release];
#endif

		// Make sure the run loop wakes up again afterwards
		_runLoop->WakeUp();

#if RN_ENABLE_VTUNE
		__itt_frame_end_v3(VTuneDomain, nullptr);
#endif
	}

	void Kernel::HandleSystemEvents()
	{
#if RN_PLATFORM_MAC_OS
		@autoreleasepool {

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
		MSG message;
		while(PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
		{
			if(message.message == WM_INPUT)
			{
				InputManager::GetSharedInstance()->__HandleRawInput((HRAWINPUT)message.lParam);
			}

			if(message.message == WM_CLOSE || message.message == WM_DESTROY || message.message == WM_QUIT)
			{
				//TODO: Find a better way to signal the app to close or maybe just close it? This sets the ESC key to true...
				InputManager::GetSharedInstance()->_keyPressed[0x1B] = true;
			}

			TranslateMessage(&message);
			DispatchMessage(&message);
		}
#endif
#if RN_PLATFORM_LINUX
		xcb_flush(_connection);

		xcb_generic_event_t *event;
		while((event = xcb_poll_for_event(_connection)))
		{}
#endif
	}

	void Kernel::Run()
	{
		_exit = false;

		do {
			_runLoop->Run();
		} while(!_exit);
	}

	void Kernel::Exit()
	{
		_exit = true;
	}
}
