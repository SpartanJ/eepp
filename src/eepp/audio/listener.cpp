#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/listener.hpp>

namespace EE { namespace Audio {

void Listener::setGlobalVolume( float volume ) {
	Private::AudioDevice::setGlobalVolume( volume );
}

float Listener::getGlobalVolume() {
	return Private::AudioDevice::getGlobalVolume();
}

void Listener::setPosition( float x, float y, float z ) {
	setPosition( Vector3f( x, y, z ) );
}

void Listener::setPosition( const Vector3f& position ) {
	Private::AudioDevice::setPosition( position );
}

Vector3f Listener::getPosition() {
	return Private::AudioDevice::getPosition();
}

void Listener::setDirection( float x, float y, float z ) {
	setDirection( Vector3f( x, y, z ) );
}

void Listener::setDirection( const Vector3f& direction ) {
	Private::AudioDevice::setDirection( direction );
}

Vector3f Listener::getDirection() {
	return Private::AudioDevice::getDirection();
}

void Listener::setUpVector( float x, float y, float z ) {
	setUpVector( Vector3f( x, y, z ) );
}

void Listener::setUpVector( const Vector3f& upVector ) {
	Private::AudioDevice::setUpVector( upVector );
}

Vector3f Listener::getUpVector() {
	return Private::AudioDevice::getUpVector();
}

}} // namespace EE::Audio
