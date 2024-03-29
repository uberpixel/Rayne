//
//  RNOpenALWorld.h
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENALWORLD_H_
#define __RAYNE_OPENALWORLD_H_

#include "RNOpenAL.h"

#include "RNOpenALSource.h"
#include "RNOpenALListener.h"

typedef struct ALCdevice ALCdevice;
typedef struct ALCcontext ALCcontext;
namespace RN
{
	class OpenALWorld : public SceneAttachment
	{
	public:
		enum MicrophonePermissionState
		{
			MicrophonePermissionStateAuthorized,
			MicrophonePermissionStateNotDetermined,
			MicrophonePermissionStateForbidden
		};
		
		OALAPI OpenALWorld(String *outputDeviceName = nullptr);
		OALAPI ~OpenALWorld() override;
		
		OALAPI void SetInputDevice(String *inputDeviceName);
		OALAPI void SetInputAudioAsset(AudioAsset *bufferAsset);
		
		OALAPI void SetListener(OpenALListener *attachment);
		OALAPI OpenALSource *PlaySound(AudioAsset*resource);
		
		OALAPI void SetDopplerEffect(float factor, float speedOfSound = 343.3);

		OALAPI static Array *GetOutputDeviceNames();
		OALAPI static Array *GetInputDeviceNames();
		
		OALAPI static void RequestMicrophonePermission();
		OALAPI static MicrophonePermissionState GetMicrophonePermissionState();

	protected:
		void Update(float delta) override;
			
	private:
		OpenALListener *_audioListener;
		
		ALCdevice *_outputDevice;
		ALCdevice *_inputDevice;
		ALCcontext *_context;
		
		AudioAsset *_inputBuffer;
		int16 *_inputBufferTemp;
			
		RNDeclareMetaAPI(OpenALWorld, OALAPI)
	};
}

#endif /* defined(__RAYNE_OPENALWORLD_H_) */
