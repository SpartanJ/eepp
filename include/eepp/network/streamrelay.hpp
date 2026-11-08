#ifndef EE_NETWORK_STREAMRELAY_HPP
#define EE_NETWORK_STREAMRELAY_HPP
#include <eepp/network/socket.hpp>
#include <eepp/network/socketselector.hpp>
#include <functional>
#include <string>

namespace EE::Network {

/** Nonblocking, bounded bidirectional byte relay. Owns both stream endpoints; connect/accept them
 * through left()/right() before polling. No worker thread is created. Endpoints must implement
 * Socket's blocking/selector interface and send/receive raw bytes (e.g. TcpSocket, LocalSocket).
 * A filter may inspect/replace/drop each chunk and return false to close the relay. It must bound
 * any data it retains itself. Reads use 64 KiB chunks; filtered output is bounded to 128 KiB per
 * direction; a slow target suspends reading its source. Poll only after wait() on the selector
 * populated by watch(). */
template <typename Left, typename Right> class StreamRelay {
  public:
	using Filter = std::function<bool( bool fromLeft, std::string& bytes )>;

	explicit StreamRelay( Filter filter = {} ) : mFilter( std::move( filter ) ) {}

	Left& left() { return mLeft; }

	Right& right() { return mRight; }

	void watch( SocketSelector& selector ) {
		if ( mClosed )
			return;
		if ( mLeft.isBlocking() )
			mLeft.setBlocking( false );
		if ( mRight.isBlocking() )
			mRight.setBlocking( false );
		selector.add( mOutgoing.empty() ? static_cast<Socket&>( mLeft ) : mRight,
					  mOutgoing.empty() ? SocketSelector::Event::Read
										: SocketSelector::Event::Write );
		selector.add( mIncoming.empty() ? static_cast<Socket&>( mRight ) : mLeft,
					  mIncoming.empty() ? SocketSelector::Event::Read
										: SocketSelector::Event::Write );
	}

	bool poll( const SocketSelector& selector ) {
		if ( mClosed )
			return false;
		// One uninitialized read buffer serves both directions. Resize-before-receive would
		// zero 64 KiB for every small chunk; only actual bytes enter the retained queues.
		char buffer[65536];
		if ( transfer( mLeft, mRight, mOutgoing, mOutgoingOffset, true, selector, buffer ) &&
			 transfer( mRight, mLeft, mIncoming, mIncomingOffset, false, selector, buffer ) )
			return true;
		close();
		return false;
	}

	/** Close both endpoints. A closed relay cannot be reused. */
	void close() {
		mClosed = true;
		mLeft.disconnect();
		mRight.disconnect();
		mOutgoing.clear();
		mIncoming.clear();
	}

  private:
	template <typename Source, typename Target>
	bool transfer( Source& source, Target& target, std::string& bytes, size_t& offset,
				   bool fromLeft, const SocketSelector& selector, char* buffer ) {
		if ( !bytes.empty() ) {
			if ( !selector.isReady( target, SocketSelector::Event::Write ) )
				return true;
		} else {
			if ( !selector.isReady( source ) )
				return true;
			size_t count = 0;
			const auto status = source.receive( buffer, 65536, count );
			if ( status == Socket::NotReady )
				return true;
			if ( status != Socket::Done )
				return false;
			bytes.assign( buffer, count );
			if ( ( mFilter && !mFilter( fromLeft, bytes ) ) || bytes.size() > 131072 )
				return false;
			if ( bytes.empty() )
				return true;
		}
		size_t count = 0;
		const auto status = target.send( bytes.data() + offset, bytes.size() - offset, count );
		if ( status != Socket::Done && status != Socket::NotReady && status != Socket::Partial )
			return false;
		offset += count;
		if ( offset == bytes.size() ) {
			bytes.clear();
			offset = 0;
		}
		return true;
	}

	// Owned endpoints come before buffers: filters never outlive the relay they belong to.
	Left mLeft;
	Right mRight;
	std::string mOutgoing;
	std::string mIncoming;
	Filter mFilter;
	size_t mOutgoingOffset{ 0 };
	size_t mIncomingOffset{ 0 };
	bool mClosed{ false };
};

} // namespace EE::Network

#endif
