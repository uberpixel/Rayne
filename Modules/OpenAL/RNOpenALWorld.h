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

typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
namespace RN
{
	class OpenALWorld : public SceneAttachment
	{
	public:
		OALAPI OpenALWorld(String *deviceName = nullptr);
		OALAPI ~OpenALWorld() override;
			
		OALAPI void SetListener(OpenALListener *attachment);
		OALAPI OpenALSource *PlaySound(AudioAsset*resource);

		OALAPI static Array *GetDeviceNames();

	protected:
		void Update(float delta) override;
			
	private:
		OpenALListener *_audioListener;
			
		ALCdevice *_device;
		ALCcontext *_context;
			
		RNDeclareMetaAPI(OpenALWorld, OALAPI)
	};
}

#endif /* defined(__RAYNE_OPENALWORLD_H_) */
