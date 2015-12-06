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
#include "../System/RNScreen.h"
#include "../Rendering/RNRenderer.h"

namespace RN
{
	static Kernel *__sharedInstance = nullptr;

	Kernel::Kernel(Application *application, const ArgumentParser &arguments) :
		_arguments(arguments),
		_application(application),
		_exit(false)
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

		try
		{
			WorkQueue::InitializeQueues();

			_observer = new RunLoopObserver(RunLoopObserver::Activity::Finalize, true,
											std::bind(&Kernel::HandleObserver, this, std::placeholders::_1,
													  std::placeholders::_2));
			_mainThread = new Thread();
			_mainQueue = WorkQueue::GetMainQueue();

			_runLoop = _mainThread->GetRunLoop();
			_runLoop->AddObserver(_observer);

			_logger = new Logger();

			AutoreleasePool pool; // Wrap everyting into an autorelease pool from now on

			Screen::InitializeScreens();

			_fileManager = new FileCoordinator();
			_firstFrame = true;
			_frames = 0;

			_delta = 0;
			_time = 0;

			SetMaxFPS(60);
			ReadManifest();

			_application->__PrepareForWillFinishLaunching(this);
			_fileManager->__PrepareWithManifest();

			_settings = new Settings(); // Requires the FileCoordinator to have all search paths
			_logger->__LoadDefaultLoggers();

			_rendererManager = new RendererManager();
			_assetManager = new AssetManager();
			_sceneManager = new SceneManager();
			_inputManager = new InputManager();
			_moduleManager = new ModuleManager();

			Catalogue::GetSharedInstance()->RegisterPendingClasses();
			_moduleManager->LoadModules();

			_application->WillFinishLaunching(this);
			_renderer = nullptr;

			if(!_arguments.HasArgument("--headless", 'h'))
			{
				RendererDescriptor *descriptor = _application->GetPreferredRenderer();

				if(!descriptor)
				{
					descriptor = _rendererManager->GetPreferredRenderer();

					if(!descriptor)
					{
						Array *renderers = _rendererManager->GetAvailableRenderers();

						if(renderers->GetCount() > 0)
							descriptor = renderers->GetObjectAtIndex<RendererDescriptor>(0);
					}
				}

				if(descriptor)
				{
					try
					{
						_renderer = _rendererManager->ActivateRenderer(descriptor);
						RNDebug("Using renderer " << _renderer);
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
		catch(...)
		{
			__sharedInstance = nullptr;
			std::rethrow_exception(std::current_exception());
		}
	}

	void Kernel::ReadManifest()
	{
		String *path = _fileManager->GetPathForLocation(FileCoordinator::Location::RootResourcesDirectory);
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
		delete _rendererManager;

		_logger->Flush();
		delete _logger;

		delete this;
		__sharedInstance = nullptr;
	}

	void Kernel::SetMaxFPS(uint32 maxFPS)
	{
		_maxFPS = maxFPS;
		_minDelta = 1.0 / maxFPS;
	}

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

		AutoreleasePool pool;

		Clock::time_point now = Clock::now();

		auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFrame).count();
		_delta = milliseconds / 1000.0;


		if(RN_EXPECT_FALSE(_firstFrame))
		{
			HandleSystemEvents();
			FinishBootstrap();

			if(_renderer)
			{
				Window *window = _renderer->GetMainWindow();
				if(!window)
				{
					window = _renderer->CreateAWindow(Vector2(1024, 768), Screen::GetMainScreen());
					window->SetTitle(_application->GetTitle());
					window->Show();
				}
			}

			_delta = 0.0;
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

		// System event handling
		HandleSystemEvents();

		// Update input and then run scene updates
		_inputManager->Update(static_cast<float>(_delta));
		_sceneManager->Update(static_cast<float>(_delta));

		if(_renderer)
		{
			_renderer->RenderIntoWindow(_renderer->GetMainWindow(), [&] {
				_sceneManager->Render(_renderer);
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

#if RN_PLATFORM_MAC_OS
		[nsautoreleasePool release];
#endif

		// Make sure the run loop wakes up again afterwards
		_runLoop->WakeUp();
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
		while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
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
