//
//  RNAndroidState.h
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//

#ifndef __RAYNE_ANDROIDSTATE_H__
#define __RAYNE_ANDROIDSTATE_H__

#include "RNBase.h"
#include <functional>

namespace RN
{
	class Kernel;
	struct __KernelBootstrapHelper;

#if RN_PLATFORM_ANDROID
	class AndroidState
	{
	public:
		AAssetManager *GetAssetManager() const
		{
			ANativeActivity *activity = GetActivity();
			return activity? activity->assetManager : nullptr;
		}
		JavaVM *GetJavaVM() const
		{
			ANativeActivity *activity = GetActivity();
			return activity? activity->vm : nullptr;
		}
		jobject GetActivityObject() const
		{
			ANativeActivity *activity = GetActivity();
			return activity? activity->clazz : nullptr;
		}
		RNAPI bool PerformWithCurrentThreadJNIContext(const std::function<void(JNIEnv *, jobject)> &callback) const;
		RNAPI void SetWindowFlags(uint32 addFlags, uint32 removeFlags) const;
		const char *GetInternalDataPath() const
		{
			ANativeActivity *activity = GetActivity();
			return activity? activity->internalDataPath : nullptr;
		}
		const char *GetExternalDataPath() const
		{
			ANativeActivity *activity = GetActivity();
			return activity? activity->externalDataPath : nullptr;
		}
		RNAPI std::string GetPackageCodePath() const;
		ANativeWindow *GetWindow() const { return _window.load(std::memory_order_acquire); }
		int32 GetActivityState() const { return _activityState.load(std::memory_order_acquire); }
		int32 GetLiveActivityState() const { return _liveActivityState.load(std::memory_order_acquire); }
		bool GetDestroyRequested() const { return _destroyRequested.load(std::memory_order_acquire); }
		RNAPI bool RequestPermission(const char *permission, int32 requestCode) const;
		RNAPI int32 CheckSelfPermission(const char *permission) const;
		RNAPI void HandleTransientCommand(int32 cmd);

	private:
		friend class Kernel;
		friend struct __KernelBootstrapHelper;

		ANativeActivity *GetActivity() const
		{
			android_app *app = _app.load(std::memory_order_acquire);
			return app? app->activity : nullptr;
		}
		bool GetRayneMainThreadJNIContext(JNIEnv *&jniEnv, jobject &activityObject) const
		{
			jniEnv = _rayneMainThreadJNIEnv.load(std::memory_order_acquire);
			activityObject = GetActivityObject();
			return (jniEnv && activityObject);
		}

		AndroidState();
		~AndroidState();

		static void ClearPendingJNIException(JNIEnv *env);
		jclass LoadClassWithActivityClassLoader(JNIEnv *env, jobject activityObject, const char *className) const;

		void ProcessSource(android_poll_source *source);
		void SetApp(android_app *app);
		void SetRayneMainThreadJNIEnv(JNIEnv *jniEnv);
		void SynchronizeState();
		void SynchronizeState(bool windowChanged, bool didResume, bool didDestroy);
		bool DrainPendingState(const std::function<void(bool, bool, bool, bool, bool, bool, bool)> &callback);

		struct PendingState
		{
			ANativeWindow *window;
			int32 activityState;
			bool destroyRequested;
			bool windowChanged;
			bool didStart;
			bool didResume;
			bool didPause;
			bool didStop;
			bool didDestroy;
			bool didLowMemory;
			bool hasPendingState;
		};

		std::atomic<android_app *> _app;
		std::atomic<ANativeWindow *> _window;
		std::atomic<int32> _activityState;
		std::atomic<int32> _liveActivityState;
		std::atomic<bool> _destroyRequested;
		std::atomic<JNIEnv *> _rayneMainThreadJNIEnv;

		Lockable _pendingStateLock;
		PendingState _pendingState;
	};
#endif
} // namespace RN

#endif /* __RAYNE_ANDROIDSTATE_H__ */
