//
//  RNSteamAudioWorld.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOWORLD_H_
#define __RAYNE_STEAMAUDIOWORLD_H_

#include "RNSteamAudio.h"

#include "RNSteamAudioSource.h"
#include "RNSteamAudioSource.h"
#include "RNSteamAudioListener.h"

struct SoundIo;
struct SoundIoDevice;
struct SoundIoOutStream;

namespace RN
{
	class SteamAudioWorld : public SceneAttachment
	{
	public:
		SAAPI SteamAudioWorld(String *deviceName = nullptr);
		SAAPI ~SteamAudioWorld() override;
			
		SAAPI void SetListener(SteamAudioListener *attachment);
		SAAPI SteamAudioSource *PlaySound(AudioAsset*resource);

		SAAPI static Array *GetDeviceNames();

	protected:
		void Update(float delta) override;
			
	private:
		SteamAudioListener *_audioListener;

		SoundIo *_soundio;
		SoundIoDevice *_device;
		SoundIoOutStream *_outstream;

		static void WriteCallback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
		static Array *_audioSources;
			
		RNDeclareMetaAPI(SteamAudioWorld, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOWORLD_H_) */
