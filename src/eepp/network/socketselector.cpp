#include <algorithm>
#include <cerrno>
#include <eepp/core/memorymanager.hpp>
#include <eepp/core/small_vector.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <eepp/network/socket.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/system/clock.hpp>
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

// WSAPoll() (Windows Vista+) has the same contract as poll(). Unlike select(), neither limits the
// number of sockets (Winsock FD_SETSIZE) or their descriptor values (POSIX FD_SETSIZE).
static int pollSockets( pollfd* sockets, size_t count, int timeout ) {
	Clock clock;
	int remaining = timeout;
	for ( ;; ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		const int result = WSAPoll( sockets, static_cast<ULONG>( count ), remaining );
		const bool interrupted = result < 0 && WSAGetLastError() == WSAEINTR;
#else
		const int result = poll( sockets, static_cast<nfds_t>( count ), remaining );
		const bool interrupted = result < 0 && errno == EINTR;
#endif
		if ( !interrupted )
			return result;
		if ( timeout >= 0 ) {
			remaining = static_cast<int>(
				std::max<Int64>( 0, static_cast<Int64>( timeout ) -
										clock.getElapsedTime().asMicroseconds() / 1000 ) );
			if ( !remaining )
				return 0;
		}
	}
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
	add( socket, Event::Read );
}

void SocketSelector::add( Socket& socket, Event event ) {
	SocketHandle handle = socket.getHandle();

	const short events = event == Event::Read	 ? POLLIN
						 : event == Event::Write ? POLLOUT
												 : POLLIN | POLLOUT;
	if ( handle != Private::SocketImpl::invalidSocket() ) {
		if ( auto* descriptor = mImpl->find( handle ) )
			descriptor->events |= events;
		else
			mImpl->Sockets.push_back( { handle, events, 0 } );
	}
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
		pollTimeout = static_cast<int>(
			std::min<Int64>( timeoutMilliseconds, std::numeric_limits<int>::max() ) );
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
		if ( descriptor.revents & ( descriptor.events | POLLERR | POLLHUP ) )
			return true;
	}

	return false;
}

bool SocketSelector::isReady( Socket& socket ) const {
	return isReady( socket, Event::Read );
}

bool SocketSelector::isReady( Socket& socket, Event event ) const {
	SocketHandle handle = socket.getHandle();

	if ( handle == Private::SocketImpl::invalidSocket() )
		return false;

	const pollfd* descriptor = mImpl->find( handle );
	const short events = event == Event::Read	 ? POLLIN
						 : event == Event::Write ? POLLOUT
												 : POLLIN | POLLOUT;
	return descriptor && ( descriptor->revents & ( events | POLLERR | POLLHUP ) );
}

SocketSelector& SocketSelector::operator=( const SocketSelector& right ) {
	SocketSelector temp( right );

	std::swap( mImpl, temp.mImpl );

	return *this;
}

}} // namespace EE::Network
