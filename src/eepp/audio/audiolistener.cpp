#include <eepp/audio/audiolistener.hpp>
#include <eepp/audio/openal.hpp>

namespace EE { namespace Audio {

void AudioListener::setGlobalVolume( const float& Volume ) {
	EnsureALInit();

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	ALCheck( alListenerf( AL_GAIN, Volume * 0.01f ) );
	#endif
}

float AudioListener::getGlobalVolume() {
	EnsureALInit();

	float Volume = 0.f;

	ALCheck( alGetListenerf( AL_GAIN, &Volume ) );

	return Volume * 100.f;
}

void AudioListener::setPosition( const float& X, const float& Y, const float& Z ) {
	EnsureALInit();

	ALCheck( alListener3f( AL_POSITION, X, Y, Z ) );
}

void AudioListener::setPosition(const Vector3ff& Position) {
	AudioListener::setPosition( Position.x, Position.y, Position.z );
}

Vector3ff AudioListener::getPosition() {
	EnsureALInit();

	Vector3ff Position;
	ALCheck( alGetListener3f( AL_POSITION, &Position.x, &Position.y, &Position.z ) );

	return Position;
}

void AudioListener::setDirection( const float& X, const float& Y, const float& Z ) {
	EnsureALInit();

	float Orientation[] = {X, Y, Z, 0.f, 1.f, 0.f};
	ALCheck( alListenerfv( AL_ORIENTATION, Orientation ) );
}

void AudioListener::setDirection(const Vector3ff& Target) {
	AudioListener::setDirection( Target.x, Target.y, Target.z );
}

Vector3ff AudioListener::getDirection() {
	EnsureALInit();

	float Orientation[6];
	ALCheck( alGetListenerfv( AL_ORIENTATION, Orientation ) );

	return Vector3ff( Orientation[0], Orientation[1], Orientation[2] );
}

}}
