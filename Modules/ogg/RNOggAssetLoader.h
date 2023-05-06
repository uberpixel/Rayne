//
//  RNOggAssetLoader.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OGGASSETLOADER_H_
#define __RAYNE_OGGASSETLOADER_H_

#include "RNOgg.h"

namespace RN
{
	namespace vorbis
	{
		struct stb_vorbis;
	}

	class OggAssetLoader : public AssetLoader
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		Asset *Load(File *file, const LoadOptions &options) override;

	private:
		OggAssetLoader(const Config &config);

		RNDeclareMetaAPI(OggAssetLoader, OGGAPI)
	};

	class OggAudioDecoder : public AudioDecoder
	{
	public:
		friend OggAssetLoader;
		OGGAPI OggAudioDecoder(File *file);
		OGGAPI uint32 DecodeFrameToAudioAsset(AudioAsset *audioAsset) final;
		OGGAPI void Seek(float time) final;
		
	private:
		File *_file;
		vorbis::stb_vorbis *_vorbis;
		short *_buffer;
		uint8 _channelCount;
		uint8 _bytesPerSample; //datatype size * channel count
		uint32 _sampleRate;
		
		RNDeclareMetaAPI(OggAudioDecoder, OGGAPI)
	};
}


#endif /* __RAYNE_ASSIMPASSETLOADER_H_ */
