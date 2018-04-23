#define DR_MP3_IMPLEMENTATION
#include <dr_libs/dr_mp3.h>

#include <eepp/audio/soundfilereadermp3.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/core/core.hpp>
#include <algorithm>
#include <cctype>
#include <eepp/audio/mp3info.hpp>
#include <eepp/system/clock.hpp>

static size_t drmp3_func_read(void* data, void* ptr, size_t size) {
	IOStream* stream = static_cast<IOStream*>(data);
	return static_cast<std::size_t>(stream->read((char*)ptr, size));
}

static drmp3_bool32 drmp3_func_seek(void* data, int offset, drmp3_seek_origin whence) {
	IOStream* stream = static_cast<IOStream*>(data);
	switch (whence) {
		case drmp3_seek_origin_start:
			break;
		case drmp3_seek_origin_current:
			offset += stream->tell();
			break;
	}
	stream->seek(offset);
	return 1;
}

namespace EE { namespace Audio { namespace Private {

bool SoundFileReaderMp3::check(IOStream& stream) {
	drmp3 mp3;

	if ( drmp3_init( &mp3, drmp3_func_read, drmp3_func_seek, &stream, NULL ) ) {
		drmp3_uninit( &mp3 );

		return true;
	}

	return false;
}

SoundFileReaderMp3::SoundFileReaderMp3() :
	mChannelCount(0),
	mMp3(NULL)
{}

SoundFileReaderMp3::~SoundFileReaderMp3() {
	close();
}

bool SoundFileReaderMp3::open(IOStream& stream, Info& info) {
	mMp3 = eeMalloc(sizeof(drmp3));

	drmp3 * mp3 = (drmp3*)mMp3;

	Mp3Info::Info mp3info = Mp3Info( stream ).getInfo();

	stream.seek(0);

	if ( drmp3_init( mp3, drmp3_func_read, drmp3_func_seek, &stream, NULL ) ) {
		info.channelCount = mChannelCount = mp3->channels;
		info.sampleRate = mp3->sampleRate;
		info.sampleCount = mp3info.frames * DRMP3_MAX_SAMPLES_PER_FRAME;
		return true;
	}

	eeSAFE_FREE( mMp3 );

	return false;
}

void SoundFileReaderMp3::seek(Uint64 sampleOffset) {
	if ( mMp3 ) {
		drmp3_seek_to_frame( (drmp3*)mMp3, sampleOffset );
	}
}

Uint64 SoundFileReaderMp3::read(Int16* samples, Uint64 maxCount) {
	eeASSERT(mMp3);

	drmp3 * mp3 = (drmp3*)mMp3;
	Uint64 count = 0;
	while (count < maxCount) {
		int samplesToRead = static_cast<int>(maxCount - count);
		int frames = samplesToRead / mChannelCount;
		float rSamples[samplesToRead];

		long framesRead = drmp3_read_f32( mp3, frames, rSamples );

		if (framesRead > 0) {
			long samplesRead = framesRead * mChannelCount;

			for ( int i = 0; i < samplesRead; i++ )
				samples[i] = ( ( ( rSamples[i] - (-1) ) * ( SHRT_MAX - SHRT_MIN ) ) / ( 1 - (-1) ) ) + SHRT_MIN;

			count += samplesRead;
			samples += samplesRead;

			if ( framesRead != frames )
				break;
		} else {
			// error or end of file
			break;
		}
	}

	return count;
}

void SoundFileReaderMp3::close() {
	if ( mMp3 ) {
		drmp3_uninit( (drmp3*)mMp3 );
		eeSAFE_FREE( mMp3 );
		mChannelCount = 0;
	}
}

}}}
