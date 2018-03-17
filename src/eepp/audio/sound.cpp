#include <eepp/audio/sound.hpp>
#include <eepp/audio/soundbuffer.hpp>
#include <eepp/audio/alcheck.hpp>

namespace EE { namespace Audio {

Sound::Sound() :
	mBuffer(NULL)
{}

Sound::Sound(const SoundBuffer& buffer) :
	mBuffer(NULL)
{
	setBuffer(buffer);
}

Sound::Sound(const Sound& copy) :
	SoundSource(copy),
	mBuffer   (NULL)
{
	if (copy.mBuffer)
		setBuffer(*copy.mBuffer);
	setLoop(copy.getLoop());
}

Sound::~Sound() {
	stop();
	if (mBuffer)
		mBuffer->detachSound(this);
}

void Sound::play() {
	alCheck(alSourcePlay(mSource));
}

void Sound::pause() {
	alCheck(alSourcePause(mSource));
}

void Sound::stop() {
	alCheck(alSourceStop(mSource));
}

void Sound::setBuffer(const SoundBuffer& buffer) {
	// First detach from the previous buffer
	if (mBuffer) {
		stop();
		mBuffer->detachSound(this);
	}

	// Assign and use the new buffer
	mBuffer = &buffer;
	mBuffer->attachSound(this);
	alCheck(alSourcei(mSource, AL_BUFFER, mBuffer->mBuffer));
}

void Sound::setLoop(bool loop) {
	alCheck(alSourcei(mSource, AL_LOOPING, loop));
}

void Sound::setPlayingOffset(Time timeOffset) {
	alCheck(alSourcef(mSource, AL_SEC_OFFSET, timeOffset.asSeconds()));
}

const SoundBuffer* Sound::getBuffer() const {
	return mBuffer;
}

bool Sound::getLoop() const {
	ALint loop;
	alCheck(alGetSourcei(mSource, AL_LOOPING, &loop));

	return loop != 0;
}

Time Sound::getPlayingOffset() const {
	ALfloat secs = 0.f;
	alCheck(alGetSourcef(mSource, AL_SEC_OFFSET, &secs));

	return Seconds(secs);
}

Sound::Status Sound::getStatus() const {
	return SoundSource::getStatus();
}

Sound& Sound::operator =(const Sound& right) {
	// Here we don't use the copy-and-swap idiom, because it would mess up
	// the list of sound instances contained in the buffers and unnecessarily
	// destroy/create OpenAL sound sources

	// Delegate to base class, which copies all the sound attributes
	SoundSource::operator=(right);

	// Detach the sound instance from the previous buffer (if any)
	if (mBuffer) {
		stop();
		mBuffer->detachSound(this);
		mBuffer = NULL;
	}

	// Copy the remaining sound attributes
	if (right.mBuffer)
		setBuffer(*right.mBuffer);
	setLoop(right.getLoop());

	return *this;
}

void Sound::resetBuffer() {
	// First stop the sound in case it is playing
	stop();

	// Detach the buffer
	if (mBuffer) {
		alCheck(alSourcei(mSource, AL_BUFFER, 0));
		mBuffer->detachSound(this);
		mBuffer = NULL;
	}
}

}}
