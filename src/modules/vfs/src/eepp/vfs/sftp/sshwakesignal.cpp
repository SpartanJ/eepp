#include "sshwakesignal.hpp"

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#include <unistd.h>
#endif

namespace EE { namespace VFS { namespace sftp {

SSHWakeSignal::SSHWakeSignal() {
	SSHSocket::init();
#if EE_PLATFORM == EE_PLATFORM_WIN
	SocketHandle listener = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( listener == InvalidSocket )
		return;
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	int len = sizeof( address );
	if ( ::bind( listener, reinterpret_cast<sockaddr*>( &address ), len ) != 0 ||
		 ::listen( listener, 1 ) != 0 ||
		 getsockname( listener, reinterpret_cast<sockaddr*>( &address ), &len ) != 0 ) {
		SSHSocket::close( listener );
		return;
	}
	mWrite = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( mWrite == InvalidSocket ||
		 ::connect( mWrite, reinterpret_cast<sockaddr*>( &address ), len ) != 0 ) {
		SSHSocket::close( listener );
		SSHSocket::close( mWrite );
		mWrite = InvalidSocket;
		return;
	}
	mRead = ::accept( listener, nullptr, nullptr );
	SSHSocket::close( listener );
	if ( mRead == InvalidSocket ) {
		SSHSocket::close( mWrite );
		mWrite = InvalidSocket;
		return;
	}
	SSHSocket::setNonBlocking( mRead );
	SSHSocket::setNonBlocking( mWrite );
#else
	int fds[2];
	if ( ::pipe( fds ) != 0 )
		return;
	mRead = fds[0];
	mWrite = fds[1];
	SSHSocket::setNonBlocking( mRead );
	SSHSocket::setNonBlocking( mWrite );
	fcntl( mRead, F_SETFD, FD_CLOEXEC );
	fcntl( mWrite, F_SETFD, FD_CLOEXEC );
#endif
}

SSHWakeSignal::~SSHWakeSignal() {
	SSHSocket::close( mRead );
	SSHSocket::close( mWrite );
}

void SSHWakeSignal::signal() {
	char byte = 1;
#if EE_PLATFORM == EE_PLATFORM_WIN
	::send( mWrite, &byte, 1, 0 );
#else
	// A full pipe already guarantees a pending wake-up, so EAGAIN is fine.
	[[maybe_unused]] ssize_t written = ::write( mWrite, &byte, 1 );
#endif
}

void SSHWakeSignal::drain() {
	char buffer[256];
#if EE_PLATFORM == EE_PLATFORM_WIN
	while ( ::recv( mRead, buffer, sizeof( buffer ), 0 ) > 0 ) {
	}
#else
	while ( ::read( mRead, buffer, sizeof( buffer ) ) > 0 ) {
	}
#endif
}

}}} // namespace EE::VFS::sftp
