//
//  RNApplication.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNApplication.h"
#include "RNKernel.h"
#include "../Rendering/RNRenderingDevice.h"
#include "../Debug/RNLoggingEngine.h"

namespace RN
{
	Application::Application() :
		_title(nullptr)
	{

	}

	Application::~Application()
	{
		SafeRelease(_title);

#if RN_PLATFORM_WINDOWS
		_fileStream.close();
#endif
	}

	void Application::__PrepareForWillFinishLaunching(Kernel *kernel)
	{
		_title = kernel->GetManifestEntryForKey<String>(kRNManifestApplicationKey);
		_title->Retain();
	}

	void Application::WillFinishLaunching(Kernel *kernel) {}
	void Application::DidFinishLaunching(Kernel *kernel) {}
	void Application::WillExit() {}

	void Application::WillStep(float delta) {}
	void Application::DidStep(float delta) {}

	void Application::WillBecomeActive() {}
	void Application::DidBecomeActive() {}
	void Application::WillResignActive() {}
	void Application::DidResignActive() {}

	RendererDescriptor *Application::GetPreferredRenderer() const
	{
		return nullptr;
	}

	RenderingDevice *Application::GetPreferredRenderingDevice(RN::RendererDescriptor *descriptor, const Array *devices) const
	{
		return devices->GetFirstObject<RenderingDevice>();
	}
	
	Array *Application::GetLoggingEngines()
	{
		DebugLogFormatter *formatter = new DebugLogFormatter();

#if RN_PLATFORM_WINDOWS
#if RN_BUILD_DEBUG
		LoggingEngine *engine = new WideCharStreamLoggingEngine(std::wcout, true);
#else
		String *loggingFilePath = FileManager::GetSharedInstance()->GetPathForLocation(FileManager::Location::ExternalSaveDirectory);
		loggingFilePath->AppendPathComponent(RNCSTR("RNLogs.txt"));
		
		_fileStream.open(loggingFilePath->GetUTF8String());
		LoggingEngine *engine = new WideCharStreamLoggingEngine(_fileStream, true);
#endif
		engine->SetLogFormatter(formatter->Autorelease());
		return Array::WithObjects({ engine->Autorelease() });
#else
		LoggingEngine *engine = new StreamLoggingEngine(std::cout, true);
		engine->SetLogFormatter(formatter->Autorelease());
		return Array::WithObjects({ engine->Autorelease() });
#endif
	}
}
