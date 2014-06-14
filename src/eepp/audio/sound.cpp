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

Sound::Sound( const SoundBuffer& Buffer, const bool& Loop, const float& Pitch, const float& Volume, const eeVector3ff& Position ) :
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
	ALCheck( alSourcei ( mSource, AL_LOOPING,  Copy.Loop()) );
	ALCheck( alSourcef ( mSource, AL_PITCH,	Copy.Pitch()) );
	ALCheck( alSourcef ( mSource, AL_GAIN,	 	Copy.Volume() * 0.01f) );
	ALCheck( alSource3f( mSource, AL_POSITION, Copy.Position().x, Copy.Position().y, Copy.Position().z) );
}

Sound::~Sound() {
	if ( mSource ) {
		if ( mBuffer ) {
			Stop();
			ALCheck( alSourcei(mSource, AL_BUFFER, 0 ) );
		}

		ALCheck( alDeleteSources( 1, &mSource ) );
	}
}

void Sound::Play() {
	ALCheck( alSourcePlay( mSource ) );
}

void Sound::Pause() {
	ALCheck( alSourcePause( mSource ) );
}

void Sound::Stop() {
	ALCheck( alSourceStop( mSource ) );
}

void Sound::Buffer(const SoundBuffer& Buffer) {
	if ( NULL != mBuffer ) {
		Stop();
		mBuffer->DetachSound( this );
	}

	mBuffer = &Buffer;
	mBuffer->AttachSound (this );

	ALCheck( alSourcei( mSource, AL_BUFFER, mBuffer ? mBuffer->mBuffer : 0 ) );
}

void Sound::Loop( const bool& Loop ) {
	ALCheck( alSourcei( mSource, AL_LOOPING, Loop ) );
}

void Sound::Pitch( const float& Pitch ) {
	ALCheck( alSourcef( mSource, AL_PITCH, Pitch ) );
}

void Sound::Volume( const float& Volume ) {
	ALCheck( alSourcef( mSource, AL_GAIN, Volume * 0.01f ) );
}

void Sound::Position( const float& X, const float& Y, const float& Z ) {
	ALCheck( alSource3f( mSource, AL_POSITION, X, Y, Z ) );
}

void Sound::Position( const eeVector3ff& Position ) {
	this->Position( Position.x, Position.y, Position.z );
}

void Sound::MinDistance( const float& MinDistance ) {
	ALCheck( alSourcef( mSource, AL_REFERENCE_DISTANCE, MinDistance ) );
}

void Sound::Attenuation( const float& Attenuation ) {
	ALCheck( alSourcef( mSource, AL_ROLLOFF_FACTOR, Attenuation ) );
}

const SoundBuffer* Sound::Buffer() const {
	return mBuffer;
}

bool Sound::Loop() const {
	ALint Loop;
	ALCheck( alGetSourcei( mSource, AL_LOOPING, &Loop ) );

	return Loop != 0;
}

float Sound::Pitch() const {
	float Pitch;
	ALCheck( alGetSourcef( mSource, AL_PITCH, &Pitch ) );

	return Pitch;
}

float Sound::Volume() const {
	float Gain = 1;
	ALCheck( alGetSourcef( mSource, AL_GAIN, &Gain ) );

	return Gain * 100.f;
}

eeVector3ff Sound::Position() const {
	eeVector3ff Position;

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	ALCheck( alGetSource3f( mSource, AL_POSITION, &Position.x, &Position.y, &Position.z ) );
	#endif

	return Position;
}

float Sound::MinDistance() const {
	float MinDistance;
	ALCheck( alGetSourcef( mSource, AL_REFERENCE_DISTANCE, &MinDistance ) );

	return MinDistance;
}

float Sound::Attenuation() const {
	float Attenuation;
	ALCheck( alGetSourcef( mSource, AL_ROLLOFF_FACTOR, &Attenuation ) );

	return Attenuation;
}

Sound::Status Sound::GetState() const {
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

Time Sound::PlayingOffset() const {
	float secs = 0.f;

	ALCheck( alGetSourcef( mSource, AL_SEC_OFFSET, &secs ) );

	return Seconds( secs );
}

void Sound::PlayingOffset( const Time &TimeOffset ) {
	ALCheck( alSourcef( mSource, AL_SEC_OFFSET, TimeOffset.AsSeconds() ) );
}

Sound& Sound::operator =( const Sound& Other ) {
	if ( NULL != mBuffer ) {
		Stop();
		mBuffer->DetachSound( this );
		mBuffer = NULL;
	}

	// Copy the sound attributes
	if ( NULL != Other.mBuffer ) {
		Buffer( *Other.mBuffer );
	}

	Loop( Other.Loop() );
	Pitch( Other.Pitch() );
	Volume( Other.Volume() );
	Position( Other.Position() );
	SetRelativeToListener( Other.IsRelativeToListener() );
	MinDistance( Other.MinDistance()) ;
	Attenuation( Other.Attenuation() );

	return *this;
}

void Sound::SetRelativeToListener( const bool& Relative ) {
	ALCheck( alSourcei( mSource, AL_SOURCE_RELATIVE, Relative ) );
}

bool Sound::IsRelativeToListener() const {
	ALint Relative;
	ALCheck( alGetSourcei( mSource, AL_SOURCE_RELATIVE, &Relative ) );
	return Relative != 0;
}

void Sound::ResetBuffer() {
	// First stop the sound in case it is playing
	Stop();

	// Detach the buffer
	if ( NULL != mBuffer ) {
		ALCheck( alSourcei( mSource, AL_BUFFER, 0 ) );

		mBuffer->DetachSound( this );

		mBuffer = NULL;
	}
}

Sound::Status Sound::State() const {
	return GetState();
}

}}
