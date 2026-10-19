//
//  RNKernel.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "../Assets/RNAssetManager.h"
#include "../Debug/RNLogger.h"
#include "../Input/RNInputManager.h"
#include "../Modules/RNModuleManager.h"
#include "../Objects/RNDictionary.h"
#include "../Objects/RNString.h"
#include "../Rendering/RNRenderer.h"
#include "../Scene/RNSceneManager.h"
#include "../System/RNFileManager.h"
#include "../Threads/RNRunLoop.h"
#include "../Threads/RNThread.h"
#include "../Threads/RNWorkQueue.h"
#include "RNApplication.h"
#include "RNArgumentParser.h"
#include "RNBase.h"
#include "RNNotificationManager.h"
#include "RNSettings.h"

#if RN_PLATFORM_ANDROID
	#include "RNAndroidState.h"
#endif

#define kRNManifestApplicationKey RNCSTR("RNApplication")
#define kRNManifestSearchPathsKey RNCSTR("RNSearchPaths")
#define kRNManifestPreferredTextureFileExtensionKey RNCSTR("RNPreferredTextureFileExtension")

namespace RN
{
	struct __KernelBootstrapHelper;
	class PerformanceScenarioRunner;

	class Kernel
	{
	public:
		friend struct __KernelBootstrapHelper;

		RNAPI static Kernel *GetSharedInstance();

		RNAPI void Run();
		RNAPI void Exit();

		RNAPI void SetMaxFPS(uint32 maxFPS);

		double GetTotalTime() const { return _time.load(std::memory_order_relaxed); }
		float GetScaleFactor() const { return 1.0f; }

		Application *GetApplication() const { return _application; }
		Settings *GetSettings() const { return _settings; }
		PerformanceScenarioRunner *GetPerformanceScenarioRunner() const { return _performanceScenarioRunner; }
		const ArgumentParser &GetArguments() const { return _arguments; }

		template<class T>
		T *GetManifestEntryForKey(const String *key) const
		{
			Object *result = __GetManifestEntryForKey(key);
			if(!result)
				return nullptr;

			return result->Downcast<T>();
		}

		bool IsActive() const { return _isActive; }

#if RN_PLATFORM_LINUX
		xcb_connection_t *GetXCBConnection() const { return _connection; }
#endif

#if RN_PLATFORM_MAC_OS
		void __WillBecomeActive();
		void __DidBecomeActive();
		void __WillResignActive();
		void __DidResignActive();
#endif

#if RN_PLATFORM_ANDROID
		const AndroidState *GetAndroidState() const { return _androidState; }
#endif

#if RN_PLATFORM_IOS
		// Called serially on the UIKit thread, including before Initialize().
		// Suspension waits for the render worker, never the game thread.
		RNAPI static void SetApplicationSuspended(bool suspended);
		RNAPI void SetMetalLayer(void *metalLayer);
		void *GetMetalLayer() const { return _metalLayer; }
#endif

#if RN_PLATFORM_VISIONOS
		RNAPI void SetLayerRenderer(void *layerRenderer);
		void *GetLayerRenderer() const { return _layerRenderer; }
#endif

	private:
#if RN_PLATFORM_ANDROID
		Kernel(Application *app, const ArgumentParser &arguments, AndroidState *androidState);
#else
		Kernel(Application *app, const ArgumentParser &arguments);
#endif
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
		FileManager *_fileManager;
		Dictionary *_manifest;
		Settings *_settings;
		PerformanceScenarioRunner *_performanceScenarioRunner;
		Logger *_logger;
		Renderer *_renderer;
		SceneManager *_sceneManager;
		AssetManager *_assetManager;
		ModuleManager *_moduleManager;
		InputManager *_inputManager;
		NotificationManager *_notificationManager;

		Thread *_mainThread;
		RunLoop *_runLoop;
		WorkQueue *_mainQueue;

#if RN_PLATFORM_LINUX
		xcb_connection_t *_connection;
#endif
#if RN_PLATFORM_ANDROID
		void InitializeAndroid(JNIEnv *jniEnv);
		void DrainAndroidStateChanges();
		AndroidState *_androidState;
#endif
#if RN_PLATFORM_IOS
		void *_metalLayer;
#endif
#if RN_PLATFORM_VISIONOS
		void *_layerRenderer;
#endif

		RunLoopObserver *_observer;
		std::atomic<bool> _exit;

		size_t _frames;
		double _minDelta;
		uint32 _maxFPS;
		Clock::time_point _lastFrame;
		bool _firstFrame;

		std::atomic<double> _time;
		double _delta;

		bool _isActive;
	};
} // namespace RN

#endif /* __RAYME_KERNEL_H___ */
