//
//  RNKernel.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "RNBase.h"
#include "RNApplication.h"
#include "RNSettings.h"
#include "RNArgumentParser.h"
#include "../Debug/RNLogger.h"
#include "../Objects/RNDictionary.h"
#include "../Objects/RNString.h"
#include "../Input/RNInputManager.h"
#include "../Assets/RNAssetManager.h"
#include "../Modules/RNModuleManager.h"
#include "../System/RNFileCoordinator.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNRendererManager.h"
#include "../Scene/RNSceneManager.h"
#include "../Threads/RNThread.h"
#include "../Threads/RNRunLoop.h"
#include "../Threads/RNWorkQueue.h"

#define kRNManifestApplicationKey RNCSTR("RNApplication")
#define kRNManifestSearchPathsKey RNCSTR("RNSearchPaths")

namespace RN
{
	struct __KernelBootstrapHelper;

	class Kernel
	{
	public:
		friend struct __KernelBootstrapHelper;

		RNAPI static Kernel *GetSharedInstance();

		RNAPI void Run();
		RNAPI void Exit();

		RNAPI void SetMaxFPS(uint32 maxFPS);

		float GetScaleFactor() const { return 1.0f; }

		Application *GetApplication() const { return _application; }
		Settings *GetSettings() const { return _settings; }

		template<class T>
		T *GetManifestEntryForKey(const String *key) const
		{
			Object *result = __GetManifestEntryForKey(key);
			if(!result)
				return nullptr;

			return result->Downcast<T>();
		}

	private:
		Kernel(Application *app, const ArgumentParser &arguments);
		~Kernel();

		void Bootstrap();
		void ReadManifest();
		void FinishBootstrap();
		void TearDown();

		void HandleObserver(RunLoopObserver *observer, RunLoopObserver::Activity activity);
		void HandleSystemEvents();

		RNAPI Object *__GetManifestEntryForKey(const String *key) const;

		const ArgumentParser &_arguments;

		Application *_application;
		FileCoordinator *_fileManager;
		Dictionary *_manifest;
		Settings *_settings;
		Logger *_logger;
		Renderer *_renderer;
		SceneManager *_sceneManager;
		AssetManager *_assetManager;
		ModuleManager *_moduleManager;
		RendererManager *_rendererManager;
		InputManager *_inputManager;

		Thread *_mainThread;
		RunLoop *_runLoop;
		WorkQueue *_mainQueue;

		RunLoopObserver *_observer;
		std::atomic<bool> _exit;

		size_t _frames;
		double _minDelta;
		uint32 _maxFPS;
		Clock::time_point _lastFrame;
		bool _firstFrame;

		double _time;
		double _delta;
	};
}

#endif /* __RAYME_KERNEL_H___ */
