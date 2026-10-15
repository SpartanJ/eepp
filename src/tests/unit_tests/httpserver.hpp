#ifndef EE_TEST_HTTP_SERVER_HPP
#define EE_TEST_HTTP_SERVER_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <eepp/network/socketselector.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/network/uri.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace HTTPTest {

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

// A bounded-wait, portable loopback server. Each accepted socket can process multiple requests;
// independent workers allow tests to distinguish connection reuse from accidental serialization.
class Server {
  public:
	using Handler = std::function<bool( TcpSocket&, const std::string&, unsigned )>;

	explicit Server( Handler handler ) : mHandler( std::move( handler ) ) {
		mListening = mListener.listen( Socket::AnyPort, IpAddress::LocalHost ) == Socket::Done;
		if ( !mListening )
			return;
		mThread = std::thread( [this] {
			SocketSelector selector;
			selector.add( mListener );
			while ( !mStop.load() ) {
				if ( !selector.wait( Milliseconds( 10 ) ) )
					continue;
				auto socket = std::make_unique<TcpSocket>();
				if ( mListener.accept( *socket ) != Socket::Done )
					continue;
				const unsigned id = ++accepted;
				mWorkers.emplace_back( [this, socket = std::move( socket ), id] {
					socket->setBlocking( false );
					SocketSelector readable;
					readable.add( *socket );
					socket->setSendTimeout( Seconds( 2 ) );
					std::string pending;
					while ( !mStop.load() ) {
						const auto end = pending.find( "\r\n\r\n" );
						if ( end != std::string::npos ) {
							Uint64 length = 0;
							const auto field = pending.find( "content-length: " );
							if ( field != std::string::npos && field < end ) {
								String::fromString(
									length,
									pending.substr( field + 16,
													pending.find( "\r\n", field ) - field - 16 ) );
							}
							if ( pending.size() >= end + 4 + length ) {
								std::string request = pending.substr( 0, end + 4 + length );
								pending.erase( 0, end + 4 + length );
								++requests;
								mCondition.notify_all();
								socket->setBlocking( true );
								if ( !mHandler( *socket, request, id ) )
									break;
								socket->setBlocking( false );
								continue;
							}
						}
						char buffer[4096];
						std::size_t count = 0;
						const auto status = socket->receive( buffer, sizeof( buffer ), count );
						if ( status == Socket::NotReady ) {
							readable.wait( Milliseconds( 20 ) );
							continue;
						}
						if ( status != Socket::Done || pending.size() > 1024 * 1024 )
							break;
						pending.append( buffer, count );
					}
					socket->disconnect();
					++closed;
					mCondition.notify_all();
				} );
			}
		} );
	}

	~Server() {
		mStop.store( true );
		if ( mThread.joinable() )
			mThread.join();
		for ( auto& worker : mWorkers )
			worker.join();
		mListener.close();
	}

	bool isListening() const { return mListening; }

	URI uri( const std::string& path = "/" ) const {
		return URI( "http://127.0.0.1:" + String::toString( mListener.getLocalPort() ) + path );
	}

	bool waitForRequests( unsigned count ) {
		return waitFor( [&] { return requests.load() >= count; } );
	}

	bool waitForClosed( unsigned count ) {
		return waitFor( [&] { return closed.load() >= count; } );
	}

	static bool send( TcpSocket& socket, const std::string& wire ) {
		return socket.send( wire.data(), wire.size() ) == Socket::Done;
	}

	static bool respond( TcpSocket& socket, const std::string& body = "ok",
						 const std::string& fields = "" ) {
		return send( socket, "HTTP/1.1 200 OK\r\nContent-Length: " +
								 String::toString( static_cast<Uint64>( body.size() ) ) + "\r\n" +
								 fields + "\r\n" + body );
	}

	std::atomic<unsigned> accepted{ 0 };
	std::atomic<unsigned> requests{ 0 };
	std::atomic<unsigned> closed{ 0 };

  private:
	template <class Predicate> bool waitFor( Predicate predicate ) {
		std::unique_lock<std::mutex> lock( mMutex );
		// The atomic predicate may change independently of this mutex. Bounded polling avoids
		// missing a notification between evaluating it and entering the condition wait.
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 3 );
		while ( !predicate() && std::chrono::steady_clock::now() < deadline )
			mCondition.wait_for( lock, std::chrono::milliseconds( 10 ) );
		return predicate();
	}

	Handler mHandler;
	TcpListener mListener;
	std::thread mThread;
	std::vector<std::thread> mWorkers;
	std::mutex mMutex;
	std::condition_variable mCondition;
	std::atomic<bool> mStop{ false };
	bool mListening{ false };
};

} // namespace HTTPTest

#endif
