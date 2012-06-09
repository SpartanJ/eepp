#include <eepp/audio/caudiolistener.hpp>

namespace EE { namespace Audio {

void cAudioListener::GlobalVolume( const ALfloat& Volume ) {
	EnsureALInit();

	ALCheck( alListenerf( AL_GAIN, Volume * 0.01f ) );
}

ALfloat cAudioListener::GlobalVolume() {
	EnsureALInit();

	ALfloat Volume = 0.f;

	ALCheck( alGetListenerf( AL_GAIN, &Volume ) );

	return Volume * 100.f;
}

void cAudioListener::Position( const ALfloat& X, const ALfloat& Y, const ALfloat& Z ) {
	EnsureALInit();

	ALCheck( alListener3f( AL_POSITION, X, Y, Z ) );
}

void cAudioListener::Position(const Vector3AL& Position) {
	cAudioListener::Position( Position.x, Position.y, Position.z );
}

Vector3AL cAudioListener::Position() {
	EnsureALInit();

	Vector3AL Position;
	ALCheck( alGetListener3f( AL_POSITION, &Position.x, &Position.y, &Position.z ) );

	return Position;
}

void cAudioListener::Target( const ALfloat& X, const ALfloat& Y, const ALfloat& Z ) {
	EnsureALInit();

	ALfloat Orientation[] = {X, Y, Z, 0.f, 1.f, 0.f};
	ALCheck( alListenerfv( AL_ORIENTATION, Orientation ) );
}

void cAudioListener::Target(const Vector3AL& Target) {
	cAudioListener::Target( Target.x, Target.y, Target.z );
}

Vector3AL cAudioListener::Target() {
	EnsureALInit();

	ALfloat Orientation[6];
	ALCheck( alGetListenerfv( AL_ORIENTATION, Orientation ) );

	return Vector3AL( Orientation[0], Orientation[1], Orientation[2] );
}

}}
