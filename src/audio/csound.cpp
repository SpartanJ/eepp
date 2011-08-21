#include "csound.hpp"

namespace EE { namespace Audio {

cSound::cSound() : mBuffer(NULL) {
	ALCheck( alGenSources( 1, &mSource ) );
	ALCheck( alSourcei( mSource, AL_BUFFER, 0 ) );
}

cSound::cSound( const cSoundBuffer& Buffer, const bool& Loop, const eeFloat& Pitch, const eeFloat& Volume, const Vector3AL& Position ) : mBuffer(&Buffer) {
	ALCheck( alGenSources(1, &mSource) );

	ALCheck( alSourcei ( mSource, AL_BUFFER,   Buffer.mBuffer ) );
	ALCheck( alSourcei ( mSource, AL_LOOPING,  Loop ) );
	ALCheck( alSourcef ( mSource, AL_PITCH,	Pitch ) );
	ALCheck( alSourcef ( mSource, AL_GAIN, Volume * 0.01f ) );
	ALCheck( alSource3f( mSource, AL_POSITION, Position.x, Position.y, Position.z ) );
}

cSound::cSound(const cSound& Copy) :
	cAudioResource(Copy),
	mBuffer(Copy.mBuffer)
{
	ALCheck( alGenSources( 1, &mSource ) );

	ALCheck( alSourcei ( mSource, AL_BUFFER,   mBuffer ? mBuffer->mBuffer : 0) );
	ALCheck( alSourcei ( mSource, AL_LOOPING,  Copy.Loop()) );
	ALCheck( alSourcef ( mSource, AL_PITCH,	Copy.Pitch()) );
	ALCheck( alSourcef ( mSource, AL_GAIN,	 	Copy.Volume() * 0.01f) );
	ALCheck( alSource3f( mSource, AL_POSITION, Copy.Position().x, Copy.Position().y, Copy.Position().z) );
}

cSound::~cSound() {
	if ( mSource ) {
		if ( mBuffer ) {
			Stop();
			ALCheck( alSourcei(mSource, AL_BUFFER, 0 ) );
		}

		ALCheck( alDeleteSources( 1, &mSource ) );
	}
}

void cSound::Play() {
	ALCheck( alSourcePlay( mSource ) );
}

void cSound::Pause() {
	ALCheck( alSourcePause( mSource ) );
}

void cSound::Stop() {
	ALCheck( alSourceStop( mSource ) );
}

void cSound::Buffer(const cSoundBuffer& Buffer) {
	if ( NULL != mBuffer ) {
		Stop();
		mBuffer->DetachSound( this );
	}

	mBuffer = &Buffer;
	mBuffer->AttachSound (this );

	ALCheck( alSourcei( mSource, AL_BUFFER, mBuffer ? mBuffer->mBuffer : 0 ) );
}

void cSound::Loop( const bool& Loop ) {
	ALCheck( alSourcei( mSource, AL_LOOPING, Loop ) );
}

void cSound::Pitch( const eeFloat& Pitch ) {
	ALCheck( alSourcef( mSource, AL_PITCH, Pitch ) );
}

void cSound::Volume( const eeFloat& Volume ) {
	ALCheck( alSourcef( mSource, AL_GAIN, Volume * 0.01f ) );
}

void cSound::Position( const eeFloat& X, const eeFloat& Y, const eeFloat& Z ) {
	ALCheck( alSource3f( mSource, AL_POSITION, X, Y, Z ) );
}

void cSound::Position( const Vector3AL& Position ) {
	this->Position( Position.x, Position.y, Position.z );
}

void cSound::MinDistance( const eeFloat& MinDistance ) {
	ALCheck( alSourcef( mSource, AL_REFERENCE_DISTANCE, MinDistance ) );
}

void cSound::Attenuation( const eeFloat& Attenuation ) {
	ALCheck( alSourcef( mSource, AL_ROLLOFF_FACTOR, Attenuation ) );
}

const cSoundBuffer* cSound::Buffer() const {
	return mBuffer;
}

bool cSound::Loop() const {
	ALint Loop;
	ALCheck( alGetSourcei( mSource, AL_LOOPING, &Loop ) );

	return Loop != 0;
}

eeFloat cSound::Pitch() const {
	ALfloat Pitch;
	ALCheck( alGetSourcef( mSource, AL_PITCH, &Pitch ) );

	return Pitch;
}

eeFloat cSound::Volume() const {
	ALfloat Gain;
	ALCheck( alGetSourcef( mSource, AL_GAIN, &Gain ) );

	return Gain * 100.f;
}

Vector3AL cSound::Position() const {
	Vector3AL Position;
	ALCheck( alGetSource3f( mSource, AL_POSITION, &Position.x, &Position.y, &Position.z ) );

	return Position;
}

eeFloat cSound::MinDistance() const {
	ALfloat MinDistance;
	ALCheck( alGetSourcef( mSource, AL_REFERENCE_DISTANCE, &MinDistance ) );

	return MinDistance;
}

eeFloat cSound::Attenuation() const {
	ALfloat Attenuation;
	ALCheck( alGetSourcef( mSource, AL_ROLLOFF_FACTOR, &Attenuation ) );

	return Attenuation;
}

EE_SOUND_STATE cSound::GetState() const {
	ALint State;
	ALCheck( alGetSourcei( mSource, AL_SOURCE_STATE, &State ) );

	switch (State) {
		case AL_INITIAL :
		case AL_STOPPED : return SOUND_STOPPED;
		case AL_PAUSED :  return SOUND_PAUSED;
		case AL_PLAYING : return SOUND_PLAYING;
	}

	return SOUND_STOPPED;
}

Uint32 cSound::PlayingOffset() const {
	ALfloat Seconds = 0.f;
	ALCheck( alGetSourcef( mSource, AL_SEC_OFFSET, &Seconds ) );

	return static_cast<Uint32> ( Seconds * 1000 );
}

void cSound::PlayingOffset( const Uint32& TimeOffset ) {
	ALCheck( alSourcef( mSource, AL_SEC_OFFSET, TimeOffset / 1000.f ) );
}

cSound& cSound::operator =( const cSound& Other ) {
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

void cSound::SetRelativeToListener( const bool& Relative ) {
	ALCheck( alSourcei( mSource, AL_SOURCE_RELATIVE, Relative ) );
}

bool cSound::IsRelativeToListener() const {
	ALint Relative;
	ALCheck( alGetSourcei( mSource, AL_SOURCE_RELATIVE, &Relative ) );
	return Relative != 0;
}

void cSound::ResetBuffer() {
	// First stop the sound in case it is playing
	Stop();

	// Detach the buffer
	ALCheck( alSourcei( mSource, AL_BUFFER, 0 ) );

	mBuffer = NULL;
}

EE_SOUND_STATE cSound::State() const {
	return GetState();
}

}}
