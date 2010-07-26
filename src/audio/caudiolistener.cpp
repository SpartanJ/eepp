#include "caudiolistener.hpp"

namespace EE { namespace Audio {

void cAudioListener::SetGlobalVolume( const ALfloat& Volume ) {
	ALCheck( alListenerf( AL_GAIN, Volume * 0.01f ) );
}

ALfloat cAudioListener::GetGlobalVolume() {
	ALfloat Volume = 0.f;
	ALCheck( alGetListenerf( AL_GAIN, &Volume ) );

	return Volume;
}

void cAudioListener::SetPosition( const ALfloat& X, const ALfloat& Y, const ALfloat& Z ) {
	ALCheck( alListener3f( AL_POSITION, X, Y, Z ) );
}

void cAudioListener::SetPosition(const Vector3AL& Position) {
	SetPosition( Position.x, Position.y, Position.z );
}

Vector3AL cAudioListener::GetPosition() {
	Vector3AL Position;
	ALCheck( alGetListener3f( AL_POSITION, &Position.x, &Position.y, &Position.z ) );

	return Position;
}

void cAudioListener::SetTarget( const ALfloat& X, const ALfloat& Y, const ALfloat& Z ) {
	ALfloat Orientation[] = {X, Y, Z, 0.f, 1.f, 0.f};
	ALCheck( alListenerfv( AL_ORIENTATION, Orientation ) );
}

void cAudioListener::SetTarget(const Vector3AL& Target) {
	SetTarget( Target.x, Target.y, Target.z );
}

Vector3AL cAudioListener::GetTarget() {
	ALfloat Orientation[6];
	ALCheck( alGetListenerfv( AL_ORIENTATION, Orientation ) );

	return Vector3AL( Orientation[0], Orientation[1], Orientation[2] );
}

}}
