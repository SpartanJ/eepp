#include <eepp/network/localsocket.hpp>

#include <cstring>
#include <eepp/network/platform/platformimpl.hpp>
#include <eepp/system/filesystem.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#include <afunix.h>
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include <sys/un.h>
#endif

namespace EE::Network {

LocalSocket::LocalSocket() : Socket( Local ) {}

Socket::Status LocalSocket::connect( const std::string& path, System::Time timeout ) {
	disconnect();
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	(void)path;
	(void)timeout;
	return Error;
#else
	sockaddr_un address{};
	if ( !makeLocalAddress( path, &address, sizeof( address ) ) || !create() )
		return Error;
	const auto status = connectAddress( &address, sizeof( address ), timeout );
	if ( status != Done && status != NotReady ) {
#if EE_PLATFORM != EE_PLATFORM_WIN
		const int error = errno;
#endif
		disconnect();
#if EE_PLATFORM != EE_PLATFORM_WIN
		errno = error;
#endif
	}
	return status;
#endif
}

Socket::Status LocalSocket::finishConnect() const {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return Error;
#else
	return connectionStatus();
#endif
}

void LocalSocket::shutdown() {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#if EE_PLATFORM == EE_PLATFORM_WIN
	::shutdown( getHandle(), SD_BOTH );
#else
	::shutdown( getHandle(), SHUT_RDWR );
#endif
#endif
}

void LocalSocket::disconnect() {
	Socket::close();
}

Socket::Status LocalSocket::send( const void* data, size_t size ) {
	size_t sent = 0;
	return sendStream( data, size, sent );
}

Socket::Status LocalSocket::send( const void* data, size_t size, size_t& sent ) {
	return sendStream( data, size, sent );
}

Socket::Status LocalSocket::receive( void* data, size_t size, size_t& received ) {
	return receiveStream( data, size, received );
}

} // namespace EE::Network
