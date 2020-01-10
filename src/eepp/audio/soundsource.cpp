#include <eepp/audio/alcheck.hpp>
#include <eepp/audio/soundsource.hpp>

namespace EE { namespace Audio {

SoundSource::SoundSource() {
	alCheck( alGenSources( 1, &mSource ) );
	alCheck( alSourcei( mSource, AL_BUFFER, 0 ) );
}

SoundSource::SoundSource( const SoundSource& copy ) {
	alCheck( alGenSources( 1, &mSource ) );
	alCheck( alSourcei( mSource, AL_BUFFER, 0 ) );

	setPitch( copy.getPitch() );
	setVolume( copy.getVolume() );
	setPosition( copy.getPosition() );
	setRelativeToListener( copy.isRelativeToListener() );
	setMinDistance( copy.getMinDistance() );
	setAttenuation( copy.getAttenuation() );
}

SoundSource::~SoundSource() {
	alCheck( alSourcei( mSource, AL_BUFFER, 0 ) );
	alCheck( alDeleteSources( 1, &mSource ) );
}

void SoundSource::setPitch( float pitch ) {
	alCheck( alSourcef( mSource, AL_PITCH, pitch ) );
}

void SoundSource::setVolume( float volume ) {
	alCheck( alSourcef( mSource, AL_GAIN, volume * 0.01f ) );
}

void SoundSource::setPosition( float x, float y, float z ) {
	alCheck( alSource3f( mSource, AL_POSITION, x, y, z ) );
}

void SoundSource::setPosition( const Vector3f& position ) {
	setPosition( position.x, position.y, position.z );
}

void SoundSource::setRelativeToListener( bool relative ) {
	alCheck( alSourcei( mSource, AL_SOURCE_RELATIVE, relative ) );
}

void SoundSource::setMinDistance( float distance ) {
	alCheck( alSourcef( mSource, AL_REFERENCE_DISTANCE, distance ) );
}

void SoundSource::setAttenuation( float attenuation ) {
	alCheck( alSourcef( mSource, AL_ROLLOFF_FACTOR, attenuation ) );
}

float SoundSource::getPitch() const {
	ALfloat pitch;
	alCheck( alGetSourcef( mSource, AL_PITCH, &pitch ) );
	return pitch;
}

float SoundSource::getVolume() const {
	ALfloat gain;
	alCheck( alGetSourcef( mSource, AL_GAIN, &gain ) );
	return gain * 100.f;
}

Vector3f SoundSource::getPosition() const {
	Vector3f position;
	alCheck( alGetSource3f( mSource, AL_POSITION, &position.x, &position.y, &position.z ) );
	return position;
}

bool SoundSource::isRelativeToListener() const {
	ALint relative;
	alCheck( alGetSourcei( mSource, AL_SOURCE_RELATIVE, &relative ) );
	return relative != 0;
}

float SoundSource::getMinDistance() const {
	ALfloat distance;
	alCheck( alGetSourcef( mSource, AL_REFERENCE_DISTANCE, &distance ) );
	return distance;
}

float SoundSource::getAttenuation() const {
	ALfloat attenuation;
	alCheck( alGetSourcef( mSource, AL_ROLLOFF_FACTOR, &attenuation ) );
	return attenuation;
}

SoundSource& SoundSource::operator=( const SoundSource& right ) {
	// Leave m_source untouched -- it's not necessary to destroy and
	// recreate the OpenAL sound source, hence no copy-and-swap idiom

	// Assign the sound attributes
	setPitch( right.getPitch() );
	setVolume( right.getVolume() );
	setPosition( right.getPosition() );
	setRelativeToListener( right.isRelativeToListener() );
	setMinDistance( right.getMinDistance() );
	setAttenuation( right.getAttenuation() );

	return *this;
}

SoundSource::Status SoundSource::getStatus() const {
	ALint status;
	alCheck( alGetSourcei( mSource, AL_SOURCE_STATE, &status ) );

	switch ( status ) {
		case AL_INITIAL:
		case AL_STOPPED:
			return Stopped;
		case AL_PAUSED:
			return Paused;
		case AL_PLAYING:
			return Playing;
	}

	return Stopped;
}

}} // namespace EE::Audio
