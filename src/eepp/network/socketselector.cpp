#include <algorithm>
#include <eepp/core/memorymanager.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <eepp/network/socket.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/system/sys.hpp>
#include <limits>
#include <utility>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <poll.h>
#endif

namespace EE { namespace Network {

struct SocketSelector::SocketSelectorImpl {
	// Inline capacity covers the UI inspector's peak (16 descriptors, it stops watching the
	// listener at its client limit) without touching the heap.
	SmallVector<pollfd, 16> Sockets; ///< Sockets and their readiness state

	pollfd* find( SocketHandle handle ) {
		for ( auto& descriptor : Sockets ) {
			if ( descriptor.fd == handle )
				return &descriptor;
		}
		return nullptr;
	}

	const pollfd* find( SocketHandle handle ) const {
		return const_cast<SocketSelectorImpl*>( this )->find( handle );
	}
};

// Mirrors select() read readiness: hang-ups and errors are reported as readable so the
// caller performs the receive() that discovers the disconnection or error. Windows reports a
// graceful peer shutdown as POLLHUP without POLLIN.
static constexpr short SOCKET_READY_EVENTS = POLLIN | POLLERR | POLLHUP;

// WSAPoll() (Windows Vista+) has the same contract as poll(). Unlike select(), neither limits the
// number of sockets (Winsock FD_SETSIZE) or their descriptor values (POSIX FD_SETSIZE).
static int pollSockets( pollfd* sockets, size_t count, int timeout ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return WSAPoll( sockets, static_cast<ULONG>( count ), timeout );
#else
	return poll( sockets, static_cast<nfds_t>( count ), timeout );
#endif
}

SocketSelector::SocketSelector() : mImpl( eeNew( SocketSelectorImpl, () ) ) {
	clear();
}

SocketSelector::SocketSelector( const SocketSelector& copy ) :
	mImpl( eeNew( SocketSelectorImpl, ( *copy.mImpl ) ) ) {}

SocketSelector::~SocketSelector() {
	eeSAFE_DELETE( mImpl );
}

void SocketSelector::add( Socket& socket ) {
	SocketHandle handle = socket.getHandle();

	if ( handle != Private::SocketImpl::invalidSocket() && !mImpl->find( handle ) )
		mImpl->Sockets.push_back( { handle, POLLIN, 0 } );
}

void SocketSelector::remove( Socket& socket ) {
	SocketHandle handle = socket.getHandle();

	if ( handle == Private::SocketImpl::invalidSocket() )
		return;

	// Order is irrelevant to poll(), so swap with the last descriptor instead of shifting.
	if ( pollfd* descriptor = mImpl->find( handle ) ) {
		*descriptor = mImpl->Sockets.back();
		mImpl->Sockets.pop_back();
	}
}

void SocketSelector::clear() {
	mImpl->Sockets.clear();
}

bool SocketSelector::wait( Time timeout ) {
	int pollTimeout = -1;
	if ( timeout != Time::Zero ) {
		// A negative poll() timeout means infinity, treat negative timeouts as an immediate check.
		Int64 timeoutMicroseconds = std::max<Int64>( 0, timeout.asMicroseconds() );
		Int64 timeoutMilliseconds = timeoutMicroseconds / 1000;
		if ( timeoutMicroseconds % 1000 != 0 )
			timeoutMilliseconds++;
		pollTimeout = timeoutMilliseconds > std::numeric_limits<int>::max()
						  ? std::numeric_limits<int>::max()
						  : static_cast<int>( timeoutMilliseconds );
	}

	// WSAPoll() fails immediately on an empty set while poll() waits for the timeout. Wait on every
	// platform so callers polling an empty selector don't spin, but never block forever: nothing
	// could ever become ready.
	if ( mImpl->Sockets.empty() ) {
		if ( pollTimeout > 0 )
			Sys::sleep( Milliseconds( pollTimeout ) );
		return false;
	}

	int count = pollSockets( mImpl->Sockets.data(), mImpl->Sockets.size(), pollTimeout );
	if ( count <= 0 ) {
		// poll() leaves revents untouched on failure, so drop readiness from a previous wait.
		if ( count < 0 ) {
			for ( auto& descriptor : mImpl->Sockets )
				descriptor.revents = 0;
		}
		return false;
	}

	for ( const auto& descriptor : mImpl->Sockets ) {
		if ( descriptor.revents & SOCKET_READY_EVENTS )
			return true;
	}

	return false;
}

bool SocketSelector::isReady( Socket& socket ) const {
	SocketHandle handle = socket.getHandle();

	if ( handle == Private::SocketImpl::invalidSocket() )
		return false;

	const pollfd* descriptor = mImpl->find( handle );
	return descriptor && ( descriptor->revents & SOCKET_READY_EVENTS );
}

SocketSelector& SocketSelector::operator=( const SocketSelector& right ) {
	SocketSelector temp( right );

	std::swap( mImpl, temp.mImpl );

	return *this;
}

}} // namespace EE::Network
