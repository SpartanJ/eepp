#include <eepp/audio/soundfilereaderflac.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/core/core.hpp>
#include <algorithm>
#include <cctype>
#define DR_FLAC_IMPLEMENTATION
#include <dr_libs/dr_flac.h>

static size_t drflac_func_read(void* data, void* ptr, size_t size) {
	IOStream* stream = static_cast<IOStream*>(data);
	return static_cast<std::size_t>(stream->read((char*)ptr, size));
}

static drflac_bool32 drflac_func_seek(void* data, int offset, drflac_seek_origin whence) {
	IOStream* stream = static_cast<IOStream*>(data);
	switch (whence) {
		case drflac_seek_origin_start:
			break;
		case drflac_seek_origin_current:
			offset += stream->tell();
			break;
	}
	stream->seek(offset);
	return 1;
}

namespace EE { namespace Audio { namespace Private {

bool SoundFileReaderFlac::check(IOStream& stream) {
	drflac * flac = drflac_open( drflac_func_read, drflac_func_seek, &stream );

	if ( flac ) {
		drflac_close( flac );

		return true;
	}

	return false;
}

SoundFileReaderFlac::SoundFileReaderFlac() :
	mChannelCount(0),
	mFlac(NULL)
{}

SoundFileReaderFlac::~SoundFileReaderFlac() {
	close();
}

bool SoundFileReaderFlac::open(IOStream& stream, Info& info) {
	stream.seek(0);

	if ( ( mFlac = drflac_open( drflac_func_read, drflac_func_seek, &stream ) ) ) {
		info.channelCount = mChannelCount = mFlac->channels;
		info.sampleRate = mFlac->sampleRate;
		info.sampleCount = mFlac->totalSampleCount;
		return true;
	}

	eeSAFE_FREE( mFlac );

	return false;
}

void SoundFileReaderFlac::seek(Uint64 sampleOffset) {
	if ( mFlac ) {
		drflac_seek_to_sample( mFlac, sampleOffset );
	}
}

Uint64 SoundFileReaderFlac::read(Int16* samples, Uint64 maxCount) {
	eeASSERT(mFlac);

	Uint64 count = 0;

	while (count < maxCount) {
		int samplesToRead = static_cast<int>(maxCount - count);
		long samplesRead = drflac_read_s16( mFlac, samplesToRead, samples );

		if (samplesRead > 0) {
			count += samplesRead;
			samples += samplesRead;
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
		eeSAFE_FREE( mFlac );
		mChannelCount = 0;
	}
}

}}}
