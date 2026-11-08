#include <eepp/network/locallistener.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/filesystem.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#include <afunix.h>
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include <sys/un.h>
#endif

namespace EE::Network {

LocalListener::LocalListener() : Socket( Local ) {}

LocalListener::~LocalListener() {
	close();
}

Socket::Status LocalListener::listen( const std::string& path, int backlog ) {
	close();
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	(void)path;
	(void)backlog;
	return Error;
#else
	sockaddr_un address{};
	if ( !makeLocalAddress( path, &address, sizeof( address ) ) || !create() )
		return Error;
	if ( ::bind( getHandle(), reinterpret_cast<sockaddr*>( &address ), sizeof( address ) ) != 0 ) {
		Socket::close();
		return Error;
	}
	mPath = path;
	const System::FileInfo identity( path, true );
	mDevice = identity.getDevice();
	mInode = identity.getInode();
	if ( ::listen( getHandle(), backlog ) != 0 ) {
		close();
		return Error;
	}
	return Done;
#endif
}

Socket::Status LocalListener::accept( LocalSocket& socket ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	(void)socket;
	return Error;
#else
	SocketHandle handle;
	do {
		handle = Private::SocketImpl::accept( getHandle(), nullptr, nullptr );
#if EE_PLATFORM == EE_PLATFORM_WIN
	} while ( handle == Private::SocketImpl::invalidSocket() && WSAGetLastError() == WSAEINTR );
#else
	} while ( handle == Private::SocketImpl::invalidSocket() && errno == EINTR );
#endif
	if ( handle == Private::SocketImpl::invalidSocket() )
		return Private::SocketImpl::getErrorStatus();
	socket.disconnect();
	socket.create( handle );
	return Done;
#endif
}

void LocalListener::close() {
	Socket::close();
	if ( !mPath.empty() ) {
		const System::FileInfo identity( mPath, true );
		if ( !System::FileInfo::inodeSupported() ||
			 ( identity.exists() && identity.getDevice() == mDevice &&
			   identity.getInode() == mInode ) ) {
			System::FileSystem::fileRemove( mPath );
		}
		mPath.clear();
	}
}

} // namespace EE::Network
