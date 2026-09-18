//
//  RNAndroidState.cpp
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//

#include "RNAndroidState.h"
#include "../Threads/RNThread.h"

namespace RN
{
#if RN_PLATFORM_ANDROID
	AndroidState::AndroidState() :
		_app(nullptr),
		_window(nullptr),
		_activityState(APP_CMD_STOP),
		_liveActivityState(APP_CMD_STOP),
		_destroyRequested(false),
		_rayneMainThreadJNIEnv(nullptr),
		_pendingState{nullptr, APP_CMD_STOP, false, false, false, false, false, false, false, false, false}
	{}

	AndroidState::~AndroidState()
	{
		_rayneMainThreadJNIEnv.store(nullptr, std::memory_order_release);

		android_app *app = _app.load(std::memory_order_acquire);
		if(app && app->userData == this)
		{
			app->userData = nullptr;
		}
	}

	bool AndroidState::PerformWithCurrentThreadJNIContext(const std::function<void(JNIEnv *, jobject)> &callback) const
	{
		if(!callback)
			return false;

		if(Thread::GetCurrentThread() == Thread::GetMainThread())
		{
			JNIEnv *env = nullptr;
			jobject activityObject = nullptr;

			if(!GetRayneMainThreadJNIContext(env, activityObject))
				return false;

			callback(env, activityObject);
			return true;
		}

		JavaVM *javaVM = GetJavaVM();
		jobject activityObject = GetActivityObject();
		if(!javaVM || !activityObject)
			return false;

		JNIEnv *env = nullptr;
		bool didAttachCurrentThread = false;
		jint result = javaVM->GetEnv(reinterpret_cast<void **>(&env), RN_JNI_VERSION_1_6);
		if(result == JNI_EDETACHED)
		{
			if(javaVM->AttachCurrentThread(&env, nullptr) != JNI_OK)
				return false;

			didAttachCurrentThread = true;
		}
		else if(result != JNI_OK || !env)
		{
			return false;
		}

		callback(env, activityObject);

		if(didAttachCurrentThread)
		{
			javaVM->DetachCurrentThread();
		}

		return true;
	}

	std::string AndroidState::GetPackageCodePath() const
	{
		std::string packageCodePath;
		PerformWithCurrentThreadJNIContext([&](JNIEnv *env, jobject activityObject) {
			jclass activityClass = nullptr;
			jstring packageCodePathString = nullptr;
			jmethodID getPackageCodePathMethod = nullptr;
			const char *packageCodePathCString = nullptr;

			ClearPendingJNIException(env);

			activityClass = env->GetObjectClass(activityObject);
			if(!activityClass)
				goto cleanup;

			getPackageCodePathMethod = env->GetMethodID(activityClass, "getPackageCodePath", "()Ljava/lang/String;");
			if(!getPackageCodePathMethod)
				goto cleanup;

			packageCodePathString = static_cast<jstring>(env->CallObjectMethod(activityObject, getPackageCodePathMethod));
			if(!packageCodePathString)
				goto cleanup;

			packageCodePathCString = env->GetStringUTFChars(packageCodePathString, nullptr);
			if(packageCodePathCString)
			{
				packageCodePath = packageCodePathCString;
				env->ReleaseStringUTFChars(packageCodePathString, packageCodePathCString);
			}

		cleanup:
			if(packageCodePathString) env->DeleteLocalRef(packageCodePathString);
			if(activityClass) env->DeleteLocalRef(activityClass);
			if(packageCodePath.empty()) ClearPendingJNIException(env);
		});

		return packageCodePath;
	}

	void AndroidState::ClearPendingJNIException(JNIEnv *env)
	{
		if(env && env->ExceptionCheck())
		{
			env->ExceptionDescribe();
			env->ExceptionClear();
		}
	}

	jclass AndroidState::LoadClassWithActivityClassLoader(JNIEnv *env, jobject activityObject, const char *className) const
	{
		jclass loadedClass = nullptr;
		jclass activityClass = nullptr;
		jobject classLoaderObject = nullptr;
		jclass classLoaderClass = nullptr;
		jmethodID getClassLoaderMethod = nullptr;
		jmethodID loadClassMethod = nullptr;
		jstring classNameString = nullptr;

		if(!env || !activityObject || !className)
			return nullptr;

		activityClass = env->GetObjectClass(activityObject);
		if(!activityClass)
			goto cleanup;

		getClassLoaderMethod = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
		if(!getClassLoaderMethod) goto cleanup;

		classLoaderObject = env->CallObjectMethod(activityObject, getClassLoaderMethod);
		if(!classLoaderObject) goto cleanup;

		classLoaderClass = env->FindClass("java/lang/ClassLoader");
		if(!classLoaderClass) goto cleanup;

		loadClassMethod = env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
		if(!loadClassMethod) goto cleanup;

		classNameString = env->NewStringUTF(className);
		if(classNameString)
		{
			loadedClass = reinterpret_cast<jclass>(env->CallObjectMethod(classLoaderObject, loadClassMethod, classNameString));
		}

	cleanup:
		if(classNameString) env->DeleteLocalRef(classNameString);
		if(classLoaderClass) env->DeleteLocalRef(classLoaderClass);
		if(classLoaderObject) env->DeleteLocalRef(classLoaderObject);
		if(activityClass) env->DeleteLocalRef(activityClass);
		if(!loadedClass) ClearPendingJNIException(env);

		return loadedClass;
	}

	void AndroidState::ProcessSource(android_poll_source *source)
	{
		android_app *app = _app.load(std::memory_order_acquire);
		if(app && source)
		{
			source->process(app, source);

			if(source->id == LOOPER_ID_MAIN)
			{
				SynchronizeState();
			}
		}
	}

	void AndroidState::SetApp(android_app *app)
	{
		android_app *oldApp = _app.load(std::memory_order_acquire);
		if(oldApp && oldApp != app && oldApp->userData == this)
		{
			oldApp->userData = nullptr;
		}

		_app.store(app, std::memory_order_release);
		if(app) app->userData = this;

		_window.store(app? app->window : nullptr, std::memory_order_release);
		_activityState.store(app? app->activityState : APP_CMD_STOP, std::memory_order_release);
		_liveActivityState.store(app? app->activityState : APP_CMD_STOP, std::memory_order_release);
		_destroyRequested.store(app? (app->destroyRequested != 0) : false, std::memory_order_release);
	}

	void AndroidState::SetRayneMainThreadJNIEnv(JNIEnv *jniEnv)
	{
		_rayneMainThreadJNIEnv.store(jniEnv, std::memory_order_release);
	}

	void AndroidState::SynchronizeState()
	{
		SynchronizeState(false, false, false);
	}

	void AndroidState::SynchronizeState(bool windowChanged, bool didResume, bool didDestroy)
	{
		android_app *app = _app.load(std::memory_order_acquire);
		PendingState state = {
			app? app->window : nullptr,
			app? app->activityState : APP_CMD_STOP,
			app? (app->destroyRequested != 0) : false,
			windowChanged,
			false,
			didResume,
			false,
			false,
			didDestroy,
			false,
			true
		};

		_liveActivityState.store(state.activityState, std::memory_order_release);

		LockGuard<Lockable> lock(_pendingStateLock);
		int32 previousActivityState = _pendingState.hasPendingState ? _pendingState.activityState : _activityState.load(std::memory_order_acquire);
		state.didStart = previousActivityState != APP_CMD_START && state.activityState == APP_CMD_START;
		state.didPause = previousActivityState != APP_CMD_PAUSE && state.activityState == APP_CMD_PAUSE;
		state.didStop = previousActivityState != APP_CMD_STOP && state.activityState == APP_CMD_STOP;
		state.didLowMemory = _pendingState.didLowMemory;
		if(_pendingState.hasPendingState)
		{
			state.windowChanged = state.windowChanged || _pendingState.windowChanged;
			state.didStart = state.didStart || _pendingState.didStart;
			state.didResume = state.didResume || _pendingState.didResume;
			state.didPause = state.didPause || _pendingState.didPause;
			state.didStop = state.didStop || _pendingState.didStop;
			state.didDestroy = state.didDestroy || _pendingState.didDestroy;
		}
		_pendingState = state;
	}

	void AndroidState::HandleTransientCommand(int32 cmd)
	{
		if(cmd != APP_CMD_LOW_MEMORY)
			return;

		LockGuard<Lockable> lock(_pendingStateLock);
		_pendingState.didLowMemory = true;
		_pendingState.hasPendingState = true;
	}

	void AndroidState::SetWindowFlags(uint32 addFlags, uint32 removeFlags) const
	{
		ANativeActivity *activity = GetActivity();
		if(activity) ANativeActivity_setWindowFlags(activity, addFlags, removeFlags);
	}

	bool AndroidState::RequestPermission(const char *permission, int32 requestCode) const
	{
		bool success = false;

		if(!permission)
			return false;

		PerformWithCurrentThreadJNIContext([&](JNIEnv *env, jobject activityObject) {
			jclass activityCompatClass = nullptr;
			jclass stringClass = nullptr;
			jobjectArray permissions = nullptr;
			jstring permissionName = nullptr;
			jmethodID requestPermissionsMethod = nullptr;

			ClearPendingJNIException(env);

			activityCompatClass = LoadClassWithActivityClassLoader(env, activityObject, "androidx.core.app.ActivityCompat");
			if(!activityCompatClass) goto cleanup;

			stringClass = env->FindClass("java/lang/String");
			if(!stringClass) goto cleanup;

			permissions = env->NewObjectArray(1, stringClass, nullptr);
			if(!permissions) goto cleanup;

			permissionName = env->NewStringUTF(permission);
			if(!permissionName) goto cleanup;

			env->SetObjectArrayElement(permissions, 0, permissionName);
			if(env->ExceptionCheck()) goto cleanup;

			requestPermissionsMethod = env->GetStaticMethodID(activityCompatClass, "requestPermissions", "(Landroid/app/Activity;[Ljava/lang/String;I)V");
			if(!requestPermissionsMethod) goto cleanup;

			env->CallStaticVoidMethod(activityCompatClass, requestPermissionsMethod, activityObject, permissions, static_cast<jint>(requestCode));
			success = !env->ExceptionCheck();

		cleanup:
			if(permissionName) env->DeleteLocalRef(permissionName);
			if(permissions) env->DeleteLocalRef(permissions);
			if(stringClass) env->DeleteLocalRef(stringClass);
			if(activityCompatClass) env->DeleteLocalRef(activityCompatClass);
			if(!success) ClearPendingJNIException(env);
		});

		return success;
	}

	int32 AndroidState::CheckSelfPermission(const char *permission) const
	{
		int32 permissionState = -1;

		if(!permission)
			return -1;

		PerformWithCurrentThreadJNIContext([&](JNIEnv *env, jobject activityObject) {
			jclass contextCompatClass = nullptr;
			jstring permissionName = nullptr;
			jmethodID checkSelfPermissionMethod = nullptr;

			ClearPendingJNIException(env);

			contextCompatClass = LoadClassWithActivityClassLoader(env, activityObject, "androidx.core.content.ContextCompat");
			if(!contextCompatClass) goto cleanup;

			checkSelfPermissionMethod = env->GetStaticMethodID(contextCompatClass, "checkSelfPermission", "(Landroid/content/Context;Ljava/lang/String;)I");
			if(!checkSelfPermissionMethod) goto cleanup;

			permissionName = env->NewStringUTF(permission);
			if(!permissionName) goto cleanup;

			permissionState = env->CallStaticIntMethod(contextCompatClass, checkSelfPermissionMethod, activityObject, permissionName);
			if(env->ExceptionCheck()) permissionState = -1;

		cleanup:
			if(permissionName) env->DeleteLocalRef(permissionName);
			if(contextCompatClass) env->DeleteLocalRef(contextCompatClass);
			if(permissionState < 0) ClearPendingJNIException(env);
		});

		return permissionState;
	}

	bool AndroidState::DrainPendingState(const std::function<void(bool, bool, bool, bool, bool, bool, bool)> &callback)
	{
		PendingState state = {nullptr, APP_CMD_STOP, false, false, false, false, false, false, false, false, false};

		{
			LockGuard<Lockable> lock(_pendingStateLock);

			if(!_pendingState.hasPendingState)
				return false;

			state = _pendingState;
			_pendingState.didLowMemory = false;
			_pendingState.hasPendingState = false;
		}

		ANativeWindow *previousWindow = _window.load(std::memory_order_acquire);
		int32 previousActivityState = _activityState.load(std::memory_order_acquire);
		bool previousDestroyRequested = _destroyRequested.load(std::memory_order_acquire);

		_window.store(state.window, std::memory_order_release);
		_activityState.store(state.activityState, std::memory_order_release);
		_destroyRequested.store(state.destroyRequested, std::memory_order_release);
		if(callback)
		{
			callback(state.windowChanged || previousWindow != state.window,
					 state.didStart || (previousActivityState != APP_CMD_START && state.activityState == APP_CMD_START),
					 state.didResume || (previousActivityState != APP_CMD_RESUME && state.activityState == APP_CMD_RESUME),
					 state.didPause || (previousActivityState != APP_CMD_PAUSE && state.activityState == APP_CMD_PAUSE),
					 state.didStop || (previousActivityState != APP_CMD_STOP && state.activityState == APP_CMD_STOP),
					 state.didDestroy || (!previousDestroyRequested && state.destroyRequested),
					 state.didLowMemory);
		}

		return true;
	}
#endif
} // namespace RN
