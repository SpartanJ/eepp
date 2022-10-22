#include <algorithm>
#include <eepp/audio/soundbufferrecorder.hpp>
#include <iterator>

namespace EE { namespace Audio {

SoundBufferRecorder::~SoundBufferRecorder() {
	// Make sure to stop the recording thread
	stop();
}

bool SoundBufferRecorder::onStart() {
	mSamples.clear();
	mBuffer = SoundBuffer();

	return true;
}

bool SoundBufferRecorder::onProcessSamples( const Int16* samples, std::size_t sampleCount ) {
	std::copy( samples, samples + sampleCount, std::back_inserter( mSamples ) );

	return true;
}

void SoundBufferRecorder::onStop() {
	if ( !mSamples.empty() )
		mBuffer.loadFromSamples( &mSamples[0], mSamples.size(), getChannelCount(),
								 getSampleRate() );
}

const SoundBuffer& SoundBufferRecorder::getBuffer() const {
	return mBuffer;
}

}} // namespace EE::Audio
