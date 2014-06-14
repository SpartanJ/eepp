#include <eepp/audio/audiolistener.hpp>
#include <eepp/audio/openal.hpp>

namespace EE { namespace Audio {

void AudioListener::GlobalVolume( const float& Volume ) {
	EnsureALInit();

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	ALCheck( alListenerf( AL_GAIN, Volume * 0.01f ) );
	#endif
}

float AudioListener::GlobalVolume() {
	EnsureALInit();

	float Volume = 0.f;

	ALCheck( alGetListenerf( AL_GAIN, &Volume ) );

	return Volume * 100.f;
}

void AudioListener::Position( const float& X, const float& Y, const float& Z ) {
	EnsureALInit();

	ALCheck( alListener3f( AL_POSITION, X, Y, Z ) );
}

void AudioListener::Position(const eeVector3ff& Position) {
	AudioListener::Position( Position.x, Position.y, Position.z );
}

eeVector3ff AudioListener::Position() {
	EnsureALInit();

	eeVector3ff Position;
	ALCheck( alGetListener3f( AL_POSITION, &Position.x, &Position.y, &Position.z ) );

	return Position;
}

void AudioListener::Target( const float& X, const float& Y, const float& Z ) {
	EnsureALInit();

	float Orientation[] = {X, Y, Z, 0.f, 1.f, 0.f};
	ALCheck( alListenerfv( AL_ORIENTATION, Orientation ) );
}

void AudioListener::Target(const eeVector3ff& Target) {
	AudioListener::Target( Target.x, Target.y, Target.z );
}

eeVector3ff AudioListener::Target() {
	EnsureALInit();

	float Orientation[6];
	ALCheck( alGetListenerfv( AL_ORIENTATION, Orientation ) );

	return eeVector3ff( Orientation[0], Orientation[1], Orientation[2] );
}

}}
