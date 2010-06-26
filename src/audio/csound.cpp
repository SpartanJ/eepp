#include "csound.hpp"

namespace EE { namespace Audio {

cSound::cSound() : myBuffer(NULL) {
	ALCheck( alGenSources(1, &mySource) );
	ALCheck( alSourcei(mySource, AL_BUFFER, 0) );
}

cSound::cSound( const cSoundBuffer& Buffer, const bool& Loop, const eeFloat& Pitch, const eeFloat& Volume, const Vector3AL& Position ) : myBuffer(&Buffer) {
	ALCheck( alGenSources(1, &mySource) );

	ALCheck( alSourcei (mySource, AL_BUFFER,   Buffer.myBuffer) );
	ALCheck( alSourcei (mySource, AL_LOOPING,  Loop) );
	ALCheck( alSourcef (mySource, AL_PITCH,	Pitch) );
	ALCheck( alSourcef (mySource, AL_GAIN,	 Volume * 0.01f) );
	ALCheck( alSource3f(mySource, AL_POSITION, Position.x, Position.y, Position.z) );
}

cSound::cSound(const cSound& Copy) : cAudioResource(Copy), myBuffer	 (Copy.myBuffer) {
	ALCheck(alGenSources(1, &mySource));

	ALCheck( alSourcei ( mySource, AL_BUFFER,   myBuffer ? myBuffer->myBuffer : 0) );
	ALCheck( alSourcei ( mySource, AL_LOOPING,  Copy.Loop()) );
	ALCheck( alSourcef ( mySource, AL_PITCH,	Copy.Pitch()) );
	ALCheck( alSourcef ( mySource, AL_GAIN,	 Copy.Volume() * 0.01f) );
	ALCheck( alSource3f( mySource, AL_POSITION, Copy.Position().x, Copy.Position().y, Copy.Position().z) );
}

cSound::~cSound() {
	if (mySource) {
		if (myBuffer) {
			Stop();
			ALCheck(alSourcei(mySource, AL_BUFFER, 0));
		}
		ALCheck(alDeleteSources(1, &mySource));
	}
}

void cSound::Play() {
	ALCheck( alSourcePlay(mySource) );
}

void cSound::Pause() {
	ALCheck( alSourcePause(mySource) );
}

void cSound::Stop() {
	ALCheck( alSourceStop(mySource) );
}

void cSound::Buffer(const cSoundBuffer& Buffer) {
	myBuffer = &Buffer;
	ALCheck( alSourcei(mySource, AL_BUFFER, myBuffer ? myBuffer->myBuffer : 0) );
}

void cSound::Loop( const bool& Loop ) {
	ALCheck( alSourcei(mySource, AL_LOOPING, Loop) );
}

void cSound::Pitch( const eeFloat& Pitch ) {
	ALCheck( alSourcef(mySource, AL_PITCH, Pitch) );
}

void cSound::Volume( const eeFloat& Volume ) {
	ALCheck( alSourcef(mySource, AL_GAIN, Volume * 0.01f) );
}

void cSound::Position( const eeFloat& X, const eeFloat& Y, const eeFloat& Z ) {
	ALCheck( alSource3f(mySource, AL_POSITION, X, Y, Z) );
}

void cSound::Position( const Vector3AL& Position ) {
	this->Position(Position.x, Position.y, Position.z);
}

void cSound::MinDistance( const eeFloat& MinDistance ) {
	ALCheck( alSourcef(mySource, AL_REFERENCE_DISTANCE, MinDistance) );
}

void cSound::Attenuation( const eeFloat& Attenuation ) {
	ALCheck( alSourcef(mySource, AL_ROLLOFF_FACTOR, Attenuation) );
}

const cSoundBuffer* cSound::Buffer() const {
	return myBuffer;
}

bool cSound::Loop() const {
	ALint Loop;
	ALCheck( alGetSourcei(mySource, AL_LOOPING, &Loop) );
	
	return Loop != 0;
}

eeFloat cSound::Pitch() const {
	ALfloat Pitch;
	ALCheck( alGetSourcef(mySource, AL_PITCH, &Pitch) );

	return Pitch;
}

eeFloat cSound::Volume() const {
	ALfloat Gain;
	ALCheck( alGetSourcef(mySource, AL_GAIN, &Gain) );

	return Gain * 100.f;
}

Vector3AL cSound::Position() const {
	Vector3AL Position;
	ALCheck( alGetSource3f(mySource, AL_POSITION, &Position.x, &Position.y, &Position.z) );

	return Position;
}

eeFloat cSound::MinDistance() const {
	ALfloat MinDistance;
	ALCheck( alGetSourcef(mySource, AL_REFERENCE_DISTANCE, &MinDistance) );

	return MinDistance;
}

eeFloat cSound::Attenuation() const {
	ALfloat Attenuation;
	ALCheck( alGetSourcef(mySource, AL_ROLLOFF_FACTOR, &Attenuation) );

	return Attenuation;
}

EE_SOUND_STATE cSound::GetState() const {
	ALint State;
	ALCheck( alGetSourcei(mySource, AL_SOURCE_STATE, &State) );

	switch (State) {
		case AL_INITIAL :
		case AL_STOPPED : return SOUND_STOPPED;
		case AL_PAUSED :  return SOUND_PAUSED;
		case AL_PLAYING : return SOUND_PLAYING;
	}

	return SOUND_STOPPED;
}

eeFloat cSound::PlayingOffset() const {
	ALfloat Seconds = 0.f;
	ALCheck( alGetSourcef(mySource, AL_SEC_OFFSET, &Seconds) );

	return Seconds;
}

void cSound::PlayingOffset( const eeFloat& TimeOffset ) {
    ALCheck( alSourcef(mySource, AL_SEC_OFFSET, TimeOffset) );
}

cSound& cSound::operator =(const cSound& Other) {
	cSound Temp(Other);

	std::swap(mySource, Temp.mySource);
	std::swap(myBuffer, Temp.myBuffer);

	return *this;
}

void cSound::SetRelativeToListener( const bool& Relative ) {
	ALCheck( alSourcei( mySource, AL_SOURCE_RELATIVE, Relative ) );
}

bool cSound::IsRelativeToListener() const {
	ALint Relative;
	ALCheck( alGetSourcei( mySource, AL_SOURCE_RELATIVE, &Relative ) );
	return Relative != 0;
}

}}
