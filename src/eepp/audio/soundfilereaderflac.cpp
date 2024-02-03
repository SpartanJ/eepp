#include <eepp/audio/soundfilereaderflac.hpp>
#include <eepp/core/core.hpp>
#include <eepp/system/iostreammemory.hpp>
#define DR_FLAC_IMPLEMENTATION
#include <dr_libs/dr_flac.h>

static size_t drflac_func_read( void* data, void* ptr, size_t size ) {
	IOStream* stream = static_cast<IOStream*>( data );
	return static_cast<std::size_t>( stream->read( (char*)ptr, size ) );
}

static drflac_bool32 drflac_func_seek( void* data, int offset, drflac_seek_origin whence ) {
	IOStream* stream = static_cast<IOStream*>( data );
	switch ( whence ) {
		case drflac_seek_origin_start:
			break;
		case drflac_seek_origin_current:
			offset += stream->tell();
			break;
	}
	stream->seek( offset );
	return 1;
}

namespace EE { namespace Audio { namespace Private {

bool SoundFileReaderFlac::check( IOStream& stream ) {
	drflac* flac = drflac_open( drflac_func_read, drflac_func_seek, &stream, NULL );

	if ( flac ) {
		drflac_close( flac );

		return true;
	}

	return false;
}

SoundFileReaderFlac::SoundFileReaderFlac() : mChannelCount( 0 ), mFlac( NULL ) {}

SoundFileReaderFlac::~SoundFileReaderFlac() {
	close();
}

bool SoundFileReaderFlac::open( IOStream& stream, Info& info ) {
	stream.seek( 0 );

	if ( ( mFlac = drflac_open( drflac_func_read, drflac_func_seek, &stream, NULL ) ) ) {
		info.channelCount = mChannelCount = mFlac->channels;
		info.sampleRate = mFlac->sampleRate;
		info.sampleCount = mFlac->totalPCMFrameCount * mFlac->channels;
		return true;
	}

	drflac_close( mFlac );

	return false;
}

void SoundFileReaderFlac::seek( Uint64 sampleOffset ) {
	if ( mFlac ) {
		drflac_seek_to_pcm_frame( mFlac, sampleOffset / mChannelCount );
	}
}

Uint64 SoundFileReaderFlac::read( Int16* samples, Uint64 maxCount ) {
	eeASSERT( mFlac );

	Uint64 count = 0;

	while ( count < maxCount ) {
		const int samplesToRead = static_cast<int>( maxCount - count );
		int frames = samplesToRead / mChannelCount;
		long framesRead = drflac_read_pcm_frames_s16( mFlac, frames, samples );

		if ( framesRead > 0 ) {
			long samplesRead = framesRead * mChannelCount;
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

void SoundFileReaderFlac::close() {
	if ( mFlac ) {
		drflac_close( mFlac );
		mChannelCount = 0;
	}
}

}}} // namespace EE::Audio::Private
