#include <eepp/audio/mp3info.hpp>
#include <eepp/audio/soundfilereadermp3.hpp>
#include <eepp/core/core.hpp>
#include <eepp/system/iostreammemory.hpp>

#define DR_MP3_IMPLEMENTATION
#include <dr_libs/dr_mp3.h>

static size_t drmp3_func_read( void* data, void* ptr, size_t size ) {
	IOStream* stream = static_cast<IOStream*>( data );
	return static_cast<std::size_t>( stream->read( (char*)ptr, size ) );
}

static drmp3_bool32 drmp3_func_seek( void* data, int offset, drmp3_seek_origin whence ) {
	IOStream* stream = static_cast<IOStream*>( data );
	switch ( whence ) {
		case DRMP3_SEEK_SET:
			break;
		case DRMP3_SEEK_CUR:
			offset += stream->tell();
			break;
		case DRMP3_SEEK_END:
			offset = stream->getSize();
			break;
	}
	stream->seek( offset );
	return 1;
}

static drmp3_bool32 drmp3_func_tell( void* data, drmp3_int64* pCursor ) {
	IOStream* stream = static_cast<IOStream*>( data );
	*pCursor = stream->tell();
	return DRMP3_TRUE;
}

static void drmp3_func_onmeta( void* pUserData, const drmp3_metadata* pMetadata ) {}

namespace EE { namespace Audio { namespace Private {

bool SoundFileReaderMp3::check( IOStream& stream ) {
	return Mp3Info( stream ).isValidMp3();
}

bool SoundFileReaderMp3::usesFileExtension( std::string_view ext ) {
	return ext == "mp3";
}

SoundFileReaderMp3::SoundFileReaderMp3() : mChannelCount( 0 ), mMp3( NULL ) {}

SoundFileReaderMp3::~SoundFileReaderMp3() {
	close();
}

bool SoundFileReaderMp3::open( IOStream& stream, Info& info ) {
	mMp3 = (drmp3*)eeMalloc( sizeof( drmp3 ) );

	stream.seek( 0 );

	if ( drmp3_init( mMp3, drmp3_func_read, drmp3_func_seek, drmp3_func_tell, drmp3_func_onmeta,
					 &stream, NULL ) ) {
		info.channelCount = mChannelCount = mMp3->channels;
		info.sampleRate = mMp3->sampleRate;
		info.sampleCount = drmp3_get_pcm_frame_count( mMp3 ) * mMp3->channels;
		return true;
	}

	eeSAFE_FREE( mMp3 );

	return false;
}

void SoundFileReaderMp3::seek( Uint64 sampleOffset ) {
	if ( mMp3 ) {
		drmp3_seek_to_pcm_frame( mMp3, sampleOffset / mChannelCount );
	}
}

Uint64 SoundFileReaderMp3::read( Int16* samples, Uint64 maxCount ) {
	eeASSERT( mMp3 );

	Uint64 count = 0;
	while ( count < maxCount ) {
		const int samplesToRead = static_cast<int>( maxCount - count );
		int frames = samplesToRead / mChannelCount;
		long framesRead = drmp3_read_pcm_frames_s16( mMp3, frames, samples );

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

void SoundFileReaderMp3::close() {
	if ( mMp3 ) {
		drmp3_uninit( mMp3 );
		eeSAFE_FREE( mMp3 );
		mChannelCount = 0;
	}
}

}}} // namespace EE::Audio::Private
