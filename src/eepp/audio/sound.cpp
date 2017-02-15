#include <eepp/audio/sound.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/openal.hpp>

namespace EE { namespace Audio {

Sound::Sound() :
	mBuffer(NULL)
{
	EnsureALInit();

	ALCheck( alGenSources( 1, &mSource ) );
	ALCheck( alSourcei( mSource, AL_BUFFER, 0 ) );
}

Sound::Sound( const SoundBuffer& Buffer, const bool& Loop, const float& Pitch, const float& Volume, const Vector3ff& Position ) :
	mBuffer(&Buffer)
{
	EnsureALInit();

	ALCheck( alGenSources(1, &mSource) );

	ALCheck( alSourcei ( mSource, AL_BUFFER,   Buffer.mBuffer ) );
	ALCheck( alSourcei ( mSource, AL_LOOPING,  Loop ) );
	ALCheck( alSourcef ( mSource, AL_PITCH,	Pitch ) );
	ALCheck( alSourcef ( mSource, AL_GAIN, Volume * 0.01f ) );
	ALCheck( alSource3f( mSource, AL_POSITION, Position.x, Position.y, Position.z ) );
}

Sound::Sound(const Sound& Copy) :
	mBuffer(Copy.mBuffer)
{
	EnsureALInit();

	ALCheck( alGenSources( 1, &mSource ) );

	ALCheck( alSourcei ( mSource, AL_BUFFER,   mBuffer ? mBuffer->mBuffer : 0) );
	ALCheck( alSourcei ( mSource, AL_LOOPING,  Copy.loop()) );
	ALCheck( alSourcef ( mSource, AL_PITCH,	Copy.pitch()) );
	ALCheck( alSourcef ( mSource, AL_GAIN,	 	Copy.volume() * 0.01f) );
	ALCheck( alSource3f( mSource, AL_POSITION, Copy.position().x, Copy.position().y, Copy.position().z) );
}

Sound::~Sound() {
	if ( mSource ) {
		if ( mBuffer ) {
			stop();
			ALCheck( alSourcei(mSource, AL_BUFFER, 0 ) );
		}

		ALCheck( alDeleteSources( 1, &mSource ) );
	}
}

void Sound::play() {
	ALCheck( alSourcePlay( mSource ) );
}

void Sound::pause() {
	ALCheck( alSourcePause( mSource ) );
}

void Sound::stop() {
	ALCheck( alSourceStop( mSource ) );
}

void Sound::buffer(const SoundBuffer& Buffer) {
	if ( NULL != mBuffer ) {
		stop();
		mBuffer->detachSound( this );
	}

	mBuffer = &Buffer;
	mBuffer->attachSound (this );

	ALCheck( alSourcei( mSource, AL_BUFFER, mBuffer ? mBuffer->mBuffer : 0 ) );
}

void Sound::loop( const bool& Loop ) {
	ALCheck( alSourcei( mSource, AL_LOOPING, Loop ) );
}

void Sound::pitch( const float& Pitch ) {
	ALCheck( alSourcef( mSource, AL_PITCH, Pitch ) );
}

void Sound::volume( const float& Volume ) {
	ALCheck( alSourcef( mSource, AL_GAIN, Volume * 0.01f ) );
}

void Sound::position( const float& X, const float& Y, const float& Z ) {
	ALCheck( alSource3f( mSource, AL_POSITION, X, Y, Z ) );
}

void Sound::position( const Vector3ff& Position ) {
	this->position( Position.x, Position.y, Position.z );
}

void Sound::minDistance( const float& MinDistance ) {
	ALCheck( alSourcef( mSource, AL_REFERENCE_DISTANCE, MinDistance ) );
}

void Sound::attenuation( const float& Attenuation ) {
	ALCheck( alSourcef( mSource, AL_ROLLOFF_FACTOR, Attenuation ) );
}

const SoundBuffer* Sound::buffer() const {
	return mBuffer;
}

bool Sound::loop() const {
	ALint Loop;
	ALCheck( alGetSourcei( mSource, AL_LOOPING, &Loop ) );

	return Loop != 0;
}

float Sound::pitch() const {
	float Pitch;
	ALCheck( alGetSourcef( mSource, AL_PITCH, &Pitch ) );

	return Pitch;
}

float Sound::volume() const {
	float Gain = 1;
	ALCheck( alGetSourcef( mSource, AL_GAIN, &Gain ) );

	return Gain * 100.f;
}

Vector3ff Sound::position() const {
	Vector3ff Position;

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	ALCheck( alGetSource3f( mSource, AL_POSITION, &Position.x, &Position.y, &Position.z ) );
	#endif

	return Position;
}

float Sound::minDistance() const {
	float MinDistance;
	ALCheck( alGetSourcef( mSource, AL_REFERENCE_DISTANCE, &MinDistance ) );

	return MinDistance;
}

float Sound::attenuation() const {
	float Attenuation;
	ALCheck( alGetSourcef( mSource, AL_ROLLOFF_FACTOR, &Attenuation ) );

	return Attenuation;
}

Sound::Status Sound::getState() const {
	ALint State;
	ALCheck( alGetSourcei( mSource, AL_SOURCE_STATE, &State ) );

	switch (State) {
		case AL_INITIAL :
		case AL_STOPPED : return Sound::Stopped;
		case AL_PAUSED :  return Sound::Paused;
		case AL_PLAYING : return Sound::Playing;
	}

	return Sound::Stopped;
}

Time Sound::playingOffset() const {
	float secs = 0.f;

	ALCheck( alGetSourcef( mSource, AL_SEC_OFFSET, &secs ) );

	return Seconds( secs );
}

void Sound::playingOffset( const Time &TimeOffset ) {
	ALCheck( alSourcef( mSource, AL_SEC_OFFSET, TimeOffset.asSeconds() ) );
}

Sound& Sound::operator =( const Sound& Other ) {
	if ( NULL != mBuffer ) {
		stop();
		mBuffer->detachSound( this );
		mBuffer = NULL;
	}

	// Copy the sound attributes
	if ( NULL != Other.mBuffer ) {
		buffer( *Other.mBuffer );
	}

	loop( Other.loop() );
	pitch( Other.pitch() );
	volume( Other.volume() );
	position( Other.position() );
	setRelativeToListener( Other.isRelativeToListener() );
	minDistance( Other.minDistance()) ;
	attenuation( Other.attenuation() );

	return *this;
}

void Sound::setRelativeToListener( const bool& Relative ) {
	ALCheck( alSourcei( mSource, AL_SOURCE_RELATIVE, Relative ) );
}

bool Sound::isRelativeToListener() const {
	ALint Relative;
	ALCheck( alGetSourcei( mSource, AL_SOURCE_RELATIVE, &Relative ) );
	return Relative != 0;
}

void Sound::resetBuffer() {
	// First stop the sound in case it is playing
	stop();

	// Detach the buffer
	if ( NULL != mBuffer ) {
		ALCheck( alSourcei( mSource, AL_BUFFER, 0 ) );

		mBuffer->detachSound( this );

		mBuffer = NULL;
	}
}

Sound::Status Sound::state() const {
	return getState();
}

}}
