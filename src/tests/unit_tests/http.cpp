#include "../../eepp/network/http/httpstreamchunked.hpp"
#include "httpserver.hpp"
#include "utest.hpp"
#include <eepp/system/compression.hpp>
#include <sstream>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <eepp/network/http.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/sys.hpp>

#include "highfiledescriptors.hpp"

using namespace EE;
using namespace EE::Network;

namespace {

struct ThreadGate {
	void enterAndWait() {
		std::unique_lock<std::mutex> lock( mutex );
		entered = true;
		condition.notify_all();
		condition.wait( lock, [this] { return released; } );
	}

	bool waitUntilEntered() {
		std::unique_lock<std::mutex> lock( mutex );
		return condition.wait_for( lock, std::chrono::seconds( 5 ), [this] { return entered; } );
	}

	void release() {
		{
			std::lock_guard<std::mutex> lock( mutex );
			released = true;
		}
		condition.notify_all();
	}

	std::mutex mutex;
	std::condition_variable condition;
	bool entered{ false };
	bool released{ false };
};

} // namespace

UTEST( Http, responseHeaderLineLargerThanReceiveBuffer ) {
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );

	std::atomic<bool> serverOk{ false };
	std::thread server( [&listener, &serverOk] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		// Read and discard the HTTP request headers.
		std::string request;
		char buffer[1024];
		std::size_t received = 0;

		while ( request.find( "\r\n\r\n" ) == std::string::npos ) {
			Socket::Status st = client.receive( buffer, sizeof( buffer ), received );
			if ( st != Socket::Done )
				return;
			request.append( buffer, received );
		}

		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "X-Long: " +
									 std::string( 17000, 'a' ) +
									 "\r\nContent-Length: 5\r\n"
									 "Connection: close\r\n"
									 "\r\n"
									 "hello";

		serverOk = client.send( response.data(), response.size() ) == Socket::Done;

		client.disconnect();
	} );

	Http http( "127.0.0.1", listener.getLocalPort() );
	Http::Response response = http.sendRequest( Http::Request( "/" ), Seconds( 5 ) );

	server.join();
	listener.close();

	EXPECT_TRUE( serverOk );
	EXPECT_EQ( response.getStatus(), Http::Response::Ok );
	EXPECT_TRUE( response.getBody() == "hello" );
}

UTEST( Http, redirectSendsCookiePairWithoutSetCookieAttributes ) {
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::string redirectedRequest;
	std::atomic<bool> serverOk{ false };
	std::thread server( [&] {
		for ( int step = 0; step < 2; ++step ) {
			TcpSocket client;
			if ( listener.accept( client ) != Socket::Done )
				return;
			std::string request;
			char buffer[1024];
			std::size_t received = 0;
			while ( request.find( "\r\n\r\n" ) == std::string::npos ) {
				if ( client.receive( buffer, sizeof( buffer ), received ) != Socket::Done )
					return;
				request.append( buffer, received );
			}
			if ( step == 1 )
				redirectedRequest = std::move( request );
			const std::string response = step == 0
											 ? "HTTP/1.1 303 See Other\r\nLocation: /landing\r\n"
											   "Set-Cookie: session=new; Path=/; HttpOnly\r\n"
											   "Set-Cookie: theme=dark; Path=/\r\n"
											   "Content-Length: 0\r\nConnection: close\r\n\r\n"
											 : "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n"
											   "Connection: close\r\n\r\nok";
			if ( client.send( response.data(), response.size() ) != Socket::Done )
				return;
			client.disconnect();
		}
		serverOk = true;
	} );
	Http http( "127.0.0.1", listener.getLocalPort() );
	Http::Request request( "/login" );
	request.setField( "Cookie", "session=old; pref=1" );
	Http::Response response = http.sendRequest( request, Seconds( 5 ) );
	server.join();
	listener.close();
	EXPECT_TRUE( serverOk );
	EXPECT_EQ( response.getStatus(), Http::Response::Ok );
	EXPECT_TRUE( redirectedRequest.find( "GET /landing " ) == 0 );
	EXPECT_TRUE( redirectedRequest.find( "cookie: theme=dark; session=new; pref=1\r\n" ) !=
				 std::string::npos );
	EXPECT_TRUE( redirectedRequest.find( "HttpOnly" ) == std::string::npos );
}

UTEST( Http, failedTlsHandshakesReleaseConnectionState ) {
	if ( !SSL::SSLSocket::isSupported() )
		return;

	constexpr int Attempts = 16;
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );

	std::atomic<int> acceptedConnections{ 0 };
	std::thread server( [&] {
		for ( int i = 0; i < Attempts; ++i ) {
			TcpSocket client;
			if ( listener.accept( client ) != Socket::Done )
				return;

			char clientHello[1024];
			std::size_t received = 0;
			if ( client.receive( clientHello, sizeof( clientHello ), received ) != Socket::Done )
				return;
			acceptedConnections.fetch_add( 1, std::memory_order_release );
			client.disconnect();
		}
	} );

	Http http( "127.0.0.1", listener.getLocalPort(), true );
	for ( int i = 0; i < Attempts; ++i ) {
		Http::Request request( "/", Http::Request::Get, "", false, false );
		Http::Response response = http.sendRequest( request, Seconds( 2 ) );
		EXPECT_EQ( response.getStatus(), Http::Response::ConnectionFailed );
	}

	server.join();
	listener.close();
	EXPECT_EQ( acceptedConnections.load( std::memory_order_acquire ), Attempts );
}

UTEST( Http, poolClearAllowsCallbackReentry ) {
	Http::setThreadPool( nullptr );
	Http::Pool::getGlobal().clear();

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );

	std::thread server( [&listener] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		std::string request;
		char buffer[1024];
		std::size_t received = 0;
		while ( request.find( "\r\n\r\n" ) == std::string::npos ) {
			if ( client.receive( buffer, sizeof( buffer ), received ) != Socket::Done )
				return;
			request.append( buffer, received );
		}

		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Length: 2\r\n"
									 "Connection: close\r\n\r\n"
									 "ok";
		client.send( response.data(), response.size() );
		client.disconnect();
	} );

	const URI uri( String::format( "http://127.0.0.1:%u/", listener.getLocalPort() ) );
	auto http = Http::Pool::getGlobal().get( uri );
	std::mutex callbackMutex;
	std::condition_variable callbackCondition;
	bool callbackEntered = false;
	bool allowPoolReentry = false;
	bool callbackCompleted = false;
	bool callbackResponseOk = false;

	http->sendAsyncRequest(
		[&]( const Http&, Http::Request&, Http::Response& response ) {
			{
				std::unique_lock<std::mutex> lock( callbackMutex );
				callbackResponseOk = response.getStatus() == Http::Response::Ok;
				callbackEntered = true;
				callbackCondition.notify_all();
				callbackCondition.wait( lock, [&] { return allowPoolReentry; } );
			}

			// Pool::clear() is concurrently destroying and joining this Http. It must not hold the
			// Pool mutex while waiting for this callback.
			Http::Pool::getGlobal().get( uri );
			{
				std::lock_guard<std::mutex> lock( callbackMutex );
				callbackCompleted = true;
			}
			callbackCondition.notify_all();
		},
		Http::Request( "/" ), Seconds( 5 ) );

	http.reset(); // Leave the Pool as the only Http owner.
	{
		std::unique_lock<std::mutex> lock( callbackMutex );
		ASSERT_TRUE( callbackCondition.wait_for( lock, std::chrono::seconds( 5 ),
												 [&] { return callbackEntered; } ) );
	}

	std::thread clearThread( [] { Http::Pool::getGlobal().clear(); } );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	{
		std::lock_guard<std::mutex> lock( callbackMutex );
		allowPoolReentry = true;
	}
	callbackCondition.notify_all();

	{
		std::unique_lock<std::mutex> lock( callbackMutex );
		ASSERT_TRUE( callbackCondition.wait_for( lock, std::chrono::seconds( 5 ),
												 [&] { return callbackCompleted; } ) );
	}

	clearThread.join();
	server.join();
	listener.close();
	Http::Pool::getGlobal().clear();
	EXPECT_TRUE( callbackResponseOk );
}

UTEST( Http, sharedThreadPoolJoinsQueuedRequestsOnPoolClear ) {
	Http::setThreadPool( nullptr );
	Http::Pool::getGlobal().clear();

	auto threadPool = System::ThreadPool::createShared( 1 );
	ThreadGate workerGate;
	threadPool->run( [&] { workerGate.enterAndWait(); } );
	ASSERT_TRUE( workerGate.waitUntilEntered() );
	Http::setThreadPool( threadPool );

	const URI uri( "http://127.0.0.1:1/" );
	auto http = Http::Pool::getGlobal().get( uri );
	System::IOStreamString stream;
	const std::string outputPath =
		System::Sys::getTempPath() + "eepp-http-shared-pool-shutdown.tmp";
	System::FileSystem::fileRemove( outputPath );
	std::atomic<int> callbacks{ 0 };
	auto callback = [&]( const Http&, Http::Request&, Http::Response& ) { callbacks++; };

	http->sendAsyncRequest( callback, Http::Request( "/memory" ), Seconds( 5 ) );
	http->downloadAsyncRequest( callback, Http::Request( "/stream" ), stream, Seconds( 5 ) );
	http->downloadAsyncRequest( callback, Http::Request( "/file" ), outputPath, Seconds( 5 ) );
	http.reset();

	std::atomic<bool> clearCompleted{ false };
	std::thread clearThread( [&] {
		Http::Pool::getGlobal().clear();
		clearCompleted.store( true, std::memory_order_release );
	} );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	EXPECT_FALSE( clearCompleted.load( std::memory_order_acquire ) );

	workerGate.release();
	clearThread.join();
	EXPECT_TRUE( clearCompleted.load( std::memory_order_acquire ) );
	EXPECT_EQ( callbacks.load(), 0 );

	Http::setThreadPool( nullptr );
	threadPool.reset();
	System::FileSystem::fileRemove( outputPath );
}

UTEST( Http, sharedThreadPoolJoinsRunningRequestOnPoolClear ) {
	Http::setThreadPool( nullptr );
	Http::Pool::getGlobal().clear();

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::thread server( [&listener] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;
		char buffer[1024];
		std::size_t received = 0;
		client.receive( buffer, sizeof( buffer ), received );
		client.disconnect();
	} );

	auto threadPool = System::ThreadPool::createShared( 1 );
	Http::setThreadPool( threadPool );
	const URI uri( String::format( "http://127.0.0.1:%u/", listener.getLocalPort() ) );
	auto http = Http::Pool::getGlobal().get( uri );
	ThreadGate progressGate;
	Http::Request request( "/" );
	request.setProgressCallback( [&]( const Http&, const Http::Request&, const Http::Response&,
									  Http::Request::Status status, std::size_t, std::size_t ) {
		if ( status == Http::Request::Connected )
			progressGate.enterAndWait();
		return true;
	} );
	std::atomic<int> callbacks{ 0 };
	http->sendAsyncRequest( [&]( const Http&, Http::Request&, Http::Response& ) { callbacks++; },
							request, Seconds( 5 ) );
	http.reset();
	ASSERT_TRUE( progressGate.waitUntilEntered() );

	std::atomic<bool> clearCompleted{ false };
	std::thread clearThread( [&] {
		Http::Pool::getGlobal().clear();
		clearCompleted.store( true, std::memory_order_release );
	} );
	std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
	EXPECT_FALSE( clearCompleted.load( std::memory_order_acquire ) );

	progressGate.release();
	clearThread.join();
	server.join();
	listener.close();
	EXPECT_TRUE( clearCompleted.load( std::memory_order_acquire ) );
	EXPECT_EQ( callbacks.load(), 0 );

	Http::setThreadPool( nullptr );
	threadPool.reset();
}

UTEST( Http, sharedThreadPoolAllowsPoolClearFromCallback ) {
	Http::setThreadPool( nullptr );
	Http::Pool::getGlobal().clear();

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::thread server( [&listener] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		std::string request;
		char buffer[1024];
		std::size_t received = 0;
		while ( request.find( "\r\n\r\n" ) == std::string::npos ) {
			if ( client.receive( buffer, sizeof( buffer ), received ) != Socket::Done )
				return;
			request.append( buffer, received );
		}
		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Length: 2\r\n"
									 "Connection: close\r\n\r\nok";
		client.send( response.data(), response.size() );
		client.disconnect();
	} );

	auto threadPool = System::ThreadPool::createShared( 1 );
	Http::setThreadPool( threadPool );
	const URI uri( String::format( "http://127.0.0.1:%u/", listener.getLocalPort() ) );
	auto http = Http::Pool::getGlobal().get( uri );
	std::mutex callbackMutex;
	std::condition_variable callbackCondition;
	bool callbackCompleted = false;
	bool callbackResponseOk = false;
	std::atomic<int> queuedCallbacks{ 0 };
	ThreadGate callbackGate;
	http->sendAsyncRequest(
		[&]( const Http&, Http::Request&, Http::Response& response ) {
			callbackResponseOk = response.getStatus() == Http::Response::Ok;
			callbackGate.enterAndWait();
			Http::Pool::getGlobal().clear();
			{
				std::lock_guard<std::mutex> lock( callbackMutex );
				callbackCompleted = true;
			}
			callbackCondition.notify_all();
		},
		Http::Request( "/" ), Seconds( 5 ) );
	ASSERT_TRUE( callbackGate.waitUntilEntered() );
	// This operation is queued behind the callback on the same one-thread executor. Pool clearing
	// from the callback must not wait for work that only this worker can drain.
	http->sendAsyncRequest(
		[&]( const Http&, Http::Request&, Http::Response& ) { queuedCallbacks++; },
		Http::Request( "/queued" ), Seconds( 5 ) );
	http.reset();
	callbackGate.release();

	{
		std::unique_lock<std::mutex> lock( callbackMutex );
		ASSERT_TRUE( callbackCondition.wait_for( lock, std::chrono::seconds( 5 ),
												 [&] { return callbackCompleted; } ) );
	}
	ThreadGate completionGate;
	threadPool->run( [&] { completionGate.enterAndWait(); } );
	ASSERT_TRUE( completionGate.waitUntilEntered() );
	completionGate.release();

	server.join();
	listener.close();
	EXPECT_TRUE( callbackResponseOk );
	EXPECT_EQ( queuedCallbacks.load(), 0 );
	Http::setThreadPool( nullptr );
	threadPool.reset();
	Http::Pool::getGlobal().clear();
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( Http, tcpConnectTimeoutHandlesFdAboveFdSetSize ) {
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );

	HighFileDescriptorReservation highFds;
	if ( !highFds.isReady() )
		UTEST_SKIP( "could not reserve enough file descriptors" );

	TcpSocket client;
	Socket::Status status =
		client.connect( IpAddress::LocalHost, listener.getLocalPort(), Seconds( 5 ) );
	EXPECT_EQ( status, Socket::Done );

	if ( status == Socket::Done ) {
		TcpSocket accepted;
		EXPECT_EQ( listener.accept( accepted ), Socket::Done );
		accepted.disconnect();
	}

	client.disconnect();
	listener.close();
}
#endif

UTEST( Http, persistentResponsesReuseConnectionAcrossWorkers ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned id ) {
		return HTTPTest::Server::respond( socket, String::toString( id ) );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	Http::Response first, second;
	std::thread a( [&] { first = client->sendRequest( Http::Request( "/one" ), Seconds( 2 ) ); } );
	a.join();
	std::thread b( [&] { second = client->sendRequest( Http::Request( "/two" ), Seconds( 2 ) ); } );
	b.join();
	EXPECT_EQ( first.getStatus(), Http::Response::Ok );
	EXPECT_EQ( second.getStatus(), Http::Response::Ok );
	EXPECT_STREQ( first.getBody().c_str(), "1" );
	EXPECT_STREQ( second.getBody().c_str(), "1" );
	EXPECT_EQ( server.accepted.load(), 1u );
	pool.clear();
	EXPECT_TRUE( server.waitForClosed( 1 ) );
}

UTEST( Http, emptyAndInformationalResponsesFinishBeforeNextRequest ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string& request, unsigned ) {
		if ( request.find( "/zero " ) != std::string::npos )
			return HTTPTest::Server::respond( socket, "" );
		if ( request.find( "HEAD " ) == 0 )
			return HTTPTest::Server::send( socket,
										   "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n" );
		if ( request.find( "/204 " ) != std::string::npos )
			return HTTPTest::Server::send( socket, "HTTP/1.1 204 No Content\r\n\r\n" );
		if ( request.find( "/304 " ) != std::string::npos )
			return HTTPTest::Server::send(
				socket, "HTTP/1.1 304 Not Modified\r\nContent-Length: 100\r\n\r\n" );
		return HTTPTest::Server::send(
			socket, "HTTP/1.1 103 Early Hints\r\nLink: </a.css>\r\n\r\n"
					"HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok" );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	for ( const auto* path : { "/zero", "/head", "/204", "/304", "/hints" } ) {
		Http::Request request( path );
		if ( std::string_view( path ) == "/head" )
			request.setMethod( Http::Request::Head );
		auto response = client->sendRequest( request, Milliseconds( 500 ) );
		EXPECT_EQ( response.getStatus(),
				   std::string_view( path ) == "/204"
					   ? Http::Response::NoContent
					   : ( std::string_view( path ) == "/304" ? Http::Response::NotModified
															  : Http::Response::Ok ) );
		EXPECT_STREQ( response.getBody().c_str(),
					  std::string_view( path ) == "/hints" ? "ok" : "" );
	}
	EXPECT_EQ( server.accepted.load(), 1u );
}

UTEST( Http, chunkedTrailersCompleteBeforeConnectionReuse ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::send(
			socket, "HTTP/1.1 200 OK\r\nTransfer-Encoding: Chunked\r\n\r\n"
					"2;extension=value\r\nhe\r\n3\r\nllo\r\n0\r\nX-Trailer: yes\r\n\r\n" );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	for ( int i = 0; i < 2; ++i ) {
		auto response = client->sendRequest( Http::Request(), Milliseconds( 500 ) );
		EXPECT_EQ( response.getStatus(), Http::Response::Ok );
		EXPECT_STREQ( response.getBody().c_str(), "hello" );
		EXPECT_STREQ( response.getField( "x-trailer" ).c_str(), "yes" );
	}
	EXPECT_EQ( server.accepted.load(), 1u );
}

UTEST( Http, chunkDecoderHandlesEverySplitIncludingTrailers ) {
	const std::string wire = "3;name=value\r\nabc\r\n2\r\nde\r\n0\r\nX-End: yes\r\n\r\n";
	for ( size_t split = 0; split <= wire.size(); ++split ) {
		System::IOStreamString output;
		EE::Network::Private::HttpStreamChunked decoder( output );
		EXPECT_EQ( decoder.write( wire.data(), split ), static_cast<ios_size>( split ) );
		EXPECT_EQ( decoder.write( wire.data() + split, wire.size() - split ),
				   static_cast<ios_size>( wire.size() - split ) );
		EXPECT_TRUE( decoder.isComplete() );
		EXPECT_FALSE( decoder.hasError() );
		EXPECT_STREQ( output.getStream().c_str(), "abcde" );
		EXPECT_STREQ( decoder.getHeaderBuffer().c_str(), "X-End: yes\r\n" );
	}
}

UTEST( Http, compressedChunkedResponseConsumesTrailerAfterDecoderEnd ) {
	const std::string text( 70000, 'a' );
	System::IOStreamMemory input( text.data(), text.size() );
	System::IOStreamString encoded;
	ASSERT_EQ( System::Compression::compress( encoded, input, System::Compression::MODE_GZIP ),
			   System::Compression::OK );
	std::mutex mutex;
	std::condition_variable condition;
	unsigned bodyObserved = 0;
	std::atomic<bool> aborted{ false };
	HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned ) {
		const auto& bytes = encoded.getStream();
		std::ostringstream size;
		size << std::hex << bytes.size();
		if ( !HTTPTest::Server::send( socket, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n"
											  "Content-Encoding: gzip\r\n\r\n" +
												  size.str() + "\r\n" + bytes + "\r\n" ) )
			return false;
		{
			std::unique_lock<std::mutex> lock( mutex );
			if ( !condition.wait_for( lock, std::chrono::seconds( 2 ),
									  [&] { return bodyObserved > 0; } ) ) {
				aborted = true;
				return false;
			}
			--bodyObserved;
		}
		return HTTPTest::Server::send( socket, "0\r\nX-End: consumed\r\n\r\n" );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	for ( int i = 0; i < 2; ++i ) {
		bool notified = false;
		Http::Request request;
		request.setProgressCallback( [&]( const Http&, const Http::Request&, const Http::Response&,
										  Http::Request::Status status, size_t, size_t ) {
			if ( status == Http::Request::ContentReceived && !notified ) {
				std::lock_guard<std::mutex> lock( mutex );
				notified = true;
				++bodyObserved;
				condition.notify_all();
			}
			return true;
		} );
		auto response = client->sendRequest( request, Seconds( 3 ) );
		EXPECT_EQ( response.getStatus(), Http::Response::Ok );
		EXPECT_TRUE( response.getBody() == text );
		EXPECT_STREQ( response.getField( "x-end" ).c_str(), "consumed" );
	}
	EXPECT_FALSE( aborted.load() );
	EXPECT_EQ( server.accepted.load(), 1u );
}

UTEST( Http, malformedAndTruncatedMessagesAreNeverReused ) {
	const std::vector<std::string> responses = {
		"HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nshort",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nab",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nX-End: incomplete\r\n",
		"HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Length: 3\r\n\r\nok",
		"HTTP/1.1 200 OK\r\nContent-Length: -1\r\n\r\nok",
		"HTTP/1.1 200 OK\r\nContent-Length: 2x\r\n\r\nok",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Length: 0\r\n\r\n0\r\n\r\n",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nZ\r\nhello\r\n0\r\n\r\n" };
	for ( const auto& wire : responses ) {
		HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned id ) {
			if ( id == 1 ) {
				HTTPTest::Server::send( socket, wire );
				return false;
			}
			return HTTPTest::Server::respond( socket );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		auto bad = client->sendRequest( Http::Request(), Seconds( 1 ) );
		EXPECT_EQ( bad.getStatus(), Http::Response::InvalidResponse );
		auto good = client->sendRequest( Http::Request(), Seconds( 1 ) );
		EXPECT_EQ( good.getStatus(), Http::Response::Ok );
		EXPECT_STREQ( good.getBody().c_str(), "ok" );
		EXPECT_EQ( server.accepted.load(), 2u );
	}
}

UTEST( Http, readTimeoutAndCancellationDiscardPartialConnections ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned id ) {
		if ( id == 1 )
			return HTTPTest::Server::send( socket,
										   "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\na" );
		return HTTPTest::Server::respond( socket );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	auto timedOut = client->sendRequest( Http::Request(), Milliseconds( 80 ) );
	EXPECT_EQ( timedOut.getStatus(), Http::Response::InvalidResponse );
	Http::Request cancelled;
	cancelled.setProgressCallback(
		[]( const Http&, const Http::Request&, const Http::Response&, Http::Request::Status status,
			size_t, size_t ) { return status != Http::Request::HeaderReceived; } );
	client->sendRequest( cancelled, Seconds( 1 ) );
	EXPECT_TRUE( cancelled.isCancelled() );
	auto good = client->sendRequest( Http::Request(), Seconds( 1 ) );
	EXPECT_EQ( good.getStatus(), Http::Response::Ok );
	EXPECT_EQ( server.accepted.load(), 3u );
}

UTEST( Http, redirectBodyIsDrainedAndPostBodyIsRemovedFor303 ) {
	std::mutex mutex;
	std::string landing;
	HTTPTest::Server server( [&]( TcpSocket& socket, const std::string& request, unsigned ) {
		if ( request.find( "POST /a/start " ) == 0 ) {
			return HTTPTest::Server::send( socket,
										   "HTTP/1.1 303 See Other\r\nLocation: ../landing\r\n"
										   "Content-Length: 7\r\n\r\ndiscard" );
		}
		{
			std::lock_guard<std::mutex> lock( mutex );
			landing = request;
		}
		return HTTPTest::Server::respond( socket, "landed" );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	auto response = client->sendRequest(
		Http::Request( "/a/start", Http::Request::Post, "payload" ), Seconds( 2 ) );
	EXPECT_EQ( response.getStatus(), Http::Response::Ok );
	EXPECT_STREQ( response.getBody().c_str(), "landed" );
	std::lock_guard<std::mutex> lock( mutex );
	EXPECT_TRUE( landing.find( "GET /landing HTTP/1.1\r\n" ) == 0 );
	EXPECT_TRUE( landing.find( "payload" ) == std::string::npos );
	EXPECT_TRUE( landing.find( "content-length" ) == std::string::npos );
	EXPECT_EQ( server.accepted.load(), 1u );
}

UTEST( Http, idleConnectionsClosedByServerAreReplacedForEveryMethod ) {
	// A server keep-alive timeout: the connection is closed while idle in the pool. The FIN is
	// already readable at reuse time, so even a POST gets a fresh connection.
	for ( const auto method : { Http::Request::Get, Http::Request::Post } ) {
		HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned id ) {
			const bool sent = HTTPTest::Server::respond( socket );
			return sent && id != 1;
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
				   Http::Response::Ok );
		EXPECT_TRUE( server.waitForClosed( 1 ) );
		auto response = client->sendRequest( Http::Request( "/", method, "a=1" ), Seconds( 1 ) );
		EXPECT_EQ( response.getStatus(), Http::Response::Ok );
		EXPECT_EQ( server.accepted.load(), 2u );
	}
}

UTEST( Http, closeAfterReusedRequestRetriesGetButNeverReplaysPost ) {
	// The server closes the reused connection after reading the request, without responding:
	// the race no pre-use check can see. Only safe methods are replayed.
	for ( const auto method : { Http::Request::Get, Http::Request::Post } ) {
		std::atomic<unsigned> firstConnectionRequests{ 0 };
		HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned id ) {
			if ( id == 1 && ++firstConnectionRequests == 2 )
				return false;
			return HTTPTest::Server::respond( socket );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
				   Http::Response::Ok );
		auto response = client->sendRequest( Http::Request( "/", method, "a=1" ), Seconds( 1 ) );
		if ( method == Http::Request::Get ) {
			EXPECT_EQ( response.getStatus(), Http::Response::Ok );
			EXPECT_EQ( server.accepted.load(), 2u );
		} else {
			EXPECT_EQ( response.getStatus(), Http::Response::ConnectionFailed );
			EXPECT_EQ( server.accepted.load(), 1u );
		}
	}
}

UTEST( Http, bytesBeyondTheFramedMessageKeepResponseButDiscardConnection ) {
	struct Case {
		Http::Request::Method method;
		std::string wire;
		std::string body;
		Http::Response::Status status;
	};
	const std::vector<Case> cases = {
		{ Http::Request::Get, "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok\r\n", "ok",
		  Http::Response::Ok },
		{ Http::Request::Get,
		  "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nok\r\n0\r\n\r\njunk", "ok",
		  Http::Response::Ok },
		{ Http::Request::Head, "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nbody", "",
		  Http::Response::Ok },
		{ Http::Request::Get, "HTTP/1.1 204 No Content\r\n\r\nbody", "",
		  Http::Response::NoContent } };
	for ( const auto& test : cases ) {
		HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned id ) {
			if ( id == 1 )
				return HTTPTest::Server::send( socket, test.wire );
			return HTTPTest::Server::respond( socket );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		auto response = client->sendRequest( Http::Request( "/", test.method ), Seconds( 1 ) );
		EXPECT_EQ( response.getStatus(), test.status );
		EXPECT_STDSTREQ( response.getBody(), test.body );
		auto next = client->sendRequest( Http::Request(), Seconds( 1 ) );
		EXPECT_EQ( next.getStatus(), Http::Response::Ok );
		EXPECT_STREQ( next.getBody().c_str(), "ok" );
		EXPECT_EQ( server.accepted.load(), 2u );
	}
}

UTEST( Http, emptyCompressedBodiesAreCompleteAndReusable ) {
	for ( const std::string framing :
		  { "Content-Length: 0\r\n\r\n", "Transfer-Encoding: chunked\r\n\r\n0\r\n\r\n" } ) {
		HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned ) {
			return HTTPTest::Server::send( socket, "HTTP/1.1 200 OK\r\nContent-Encoding: gzip\r\n" +
													   framing );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		for ( int i = 0; i < 2; ++i ) {
			auto response = client->sendRequest( Http::Request(), Seconds( 1 ) );
			EXPECT_EQ( response.getStatus(), Http::Response::Ok );
			EXPECT_TRUE( response.getBody().empty() );
		}
		EXPECT_EQ( server.accepted.load(), 1u );
	}
}

UTEST( Http, chunkDecoderAcceptsWhitespaceBeforeExtensionsAndStopsAtMessageEnd ) {
	const std::string wire = "3 \t;name=value\r\nabc\r\n2\t\r\nde\r\n0 ;last\r\n\r\n";
	System::IOStreamString output;
	EE::Network::Private::HttpStreamChunked decoder( output );
	const std::string withExtra = wire + "next";
	EXPECT_EQ( decoder.write( withExtra.data(), withExtra.size() ),
			   static_cast<ios_size>( wire.size() ) );
	EXPECT_TRUE( decoder.isComplete() );
	EXPECT_FALSE( decoder.hasError() );
	EXPECT_STREQ( output.getStream().c_str(), "abcde" );
	EXPECT_EQ( decoder.write( "more", 4 ), static_cast<ios_size>( 0 ) );

	// Whitespace is only allowed between the size and an extension, never before the size.
	System::IOStreamString rejected;
	EE::Network::Private::HttpStreamChunked leading( rejected );
	EXPECT_EQ( leading.write( " 3\r\nabc\r\n", 10 ), static_cast<ios_size>( -1 ) );
	EXPECT_TRUE( leading.hasError() );
}

UTEST( Http, everySetCookieFieldIsKeptSeparately ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond(
			socket, "ok",
			"Set-Cookie: a=1; Expires=Wed, 21 Oct 2026 07:28:00 GMT\r\n"
			"Vary: Accept\r\n"
			"Set-Cookie: b=2; Path=/\r\n"
			"Vary: Cookie\r\n" );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto response = pool.get( server.uri() )->sendRequest( Http::Request(), Seconds( 1 ) );
	EXPECT_EQ( response.getStatus(), Http::Response::Ok );
	EXPECT_STREQ( response.getField( "set-cookie" ).c_str(),
				  "a=1; Expires=Wed, 21 Oct 2026 07:28:00 GMT\nb=2; Path=/" );
	EXPECT_STREQ( response.getField( "vary" ).c_str(), "Accept,Cookie" );
}

UTEST( Http, connectionAndTlsPoliciesControlReuse ) {
	for ( const auto* fields : { "Connection: Keep-Alive, CLOSE\r\n", "" } ) {
		HTTPTest::Server server( [fields]( TcpSocket& socket, const std::string&, unsigned ) {
			return HTTPTest::Server::respond( socket, "ok", fields );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		for ( bool validate : { true, false, true } ) {
			Http::Request request( "/", Http::Request::Get, "", validate, validate );
			EXPECT_EQ( client->sendRequest( request, Seconds( 1 ) ).getStatus(),
					   Http::Response::Ok );
		}
		EXPECT_EQ( server.accepted.load(), *fields ? 3u : 2u );
	}
}

UTEST( Http, poolBoundsConcurrentConnectionsAndWaitersRespectTimeouts ) {
	ThreadGate gate;
	HTTPTest::Server server( [&]( TcpSocket& socket, const std::string&, unsigned ) {
		gate.enterAndWait();
		return HTTPTest::Server::respond( socket );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::ConnectionPoolOptions options;
	options.maxConnections = 2;
	options.maxConnectionsPerHost = 2;
	Http::Pool pool( options );
	auto client = pool.get( server.uri() );
	Http::Response first, second;
	std::thread a( [&] { first = client->sendRequest( Http::Request(), Seconds( 3 ) ); } );
	std::thread b( [&] { second = client->sendRequest( Http::Request(), Seconds( 3 ) ); } );
	const bool bothEntered = server.waitForRequests( 2 );
	auto blocked = client->sendRequest( Http::Request(), Milliseconds( 50 ) );
	HTTPTest::Server other( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond( socket );
	} );
	auto otherClient = pool.get( other.uri() );
	auto globallyBlocked = otherClient->sendRequest( Http::Request(), Milliseconds( 50 ) );
	const auto accepted = server.accepted.load();
	gate.release();
	a.join();
	b.join();
	EXPECT_TRUE( bothEntered );
	EXPECT_EQ( blocked.getStatus(), Http::Response::ConnectionFailed );
	EXPECT_EQ( accepted, 2u );
	EXPECT_EQ( globallyBlocked.getStatus(), Http::Response::ConnectionFailed );
	EXPECT_EQ( other.accepted.load(), 0u );
	EXPECT_EQ( first.getStatus(), Http::Response::Ok );
	EXPECT_EQ( second.getStatus(), Http::Response::Ok );
	EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
			   Http::Response::Ok );
	EXPECT_EQ( server.accepted.load(), 2u );
}

UTEST( Http, idleExpiryAndExplicitIdleCloseReleaseSockets ) {
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond( socket );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::ConnectionPoolOptions options;
	options.idleTimeout = Time::Zero;
	Http::Pool pool( options );
	auto client = pool.get( server.uri() );
	EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
			   Http::Response::Ok );
	EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
			   Http::Response::Ok );
	EXPECT_EQ( server.accepted.load(), 2u );
	pool.closeIdleConnections();
	EXPECT_TRUE( server.waitForClosed( 2 ) );
}

UTEST( Http, proxyIsUsedAndPoolKeysSeparateOriginsAndProxies ) {
	std::mutex mutex;
	std::string seen;
	HTTPTest::Server proxy( [&]( TcpSocket& socket, const std::string& request, unsigned ) {
		{
			std::lock_guard<std::mutex> lock( mutex );
			seen = request;
		}
		return HTTPTest::Server::respond( socket );
	} );
	ASSERT_TRUE( proxy.isListening() );
	Http::Pool pool;
	const URI origin( "http://unresolvable.invalid:8081/path" );
	auto client = pool.get( origin, proxy.uri() );
	EXPECT_TRUE( client ==
				 pool.get( URI( "HTTP://UNRESOLVABLE.INVALID:8081/other" ), proxy.uri() ) );
	EXPECT_TRUE( client !=
				 pool.get( URI( "https://unresolvable.invalid:8081/path" ), proxy.uri() ) );
	EXPECT_TRUE( client != pool.get( origin ) );
	auto response = client->sendRequest( Http::Request( "/path" ), Seconds( 1 ) );
	EXPECT_EQ( response.getStatus(), Http::Response::Ok );
	{
		std::lock_guard<std::mutex> lock( mutex );
		EXPECT_TRUE( seen.find( "GET http://unresolvable.invalid:8081/path HTTP/1.1\r\n" ) == 0 );
		EXPECT_TRUE( seen.find( "host: unresolvable.invalid:8081\r\n" ) != std::string::npos );
	}
	Http direct;
	direct.setHost( origin, proxy.uri() );
	EXPECT_EQ( direct.sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
			   Http::Response::Ok );
}

UTEST( Http, globalCapacityEvictsIdleConnectionsAcrossOrigins ) {
	HTTPTest::Server first( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond( socket, "first" );
	} );
	HTTPTest::Server second( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond( socket, "second" );
	} );
	ASSERT_TRUE( first.isListening() && second.isListening() );
	Http::ConnectionPoolOptions options;
	options.maxConnections = 1;
	Http::Pool pool( options );
	auto a = pool.get( first.uri() );
	auto b = pool.get( second.uri() );
	EXPECT_TRUE( a->sendRequest( Http::Request(), Seconds( 1 ) ).getBody() == "first" );
	EXPECT_TRUE( b->sendRequest( Http::Request(), Seconds( 1 ) ).getBody() == "second" );
	EXPECT_TRUE( first.waitForClosed( 1 ) );
	EXPECT_TRUE( a->sendRequest( Http::Request(), Seconds( 1 ) ).getBody() == "first" );
	EXPECT_TRUE( second.waitForClosed( 1 ) );
	EXPECT_EQ( first.accepted.load(), 2u );
	EXPECT_EQ( second.accepted.load(), 1u );
}

UTEST( Http, http10AndExplicitCloseRespectPersistencePolicy ) {
	for ( int policy = 0; policy < 3; ++policy ) {
		HTTPTest::Server server( [policy]( TcpSocket& socket, const std::string&, unsigned ) {
			return HTTPTest::Server::send(
				socket, "HTTP/1.0 200 OK\r\nContent-Length: 2\r\n" +
							std::string( policy ? "Connection: Keep-Alive\r\n" : "" ) + "\r\nok" );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		Http::Request request;
		if ( policy == 2 )
			request.setField( "Connection", "close" );
		for ( int i = 0; i < 2; ++i ) {
			EXPECT_EQ( client->sendRequest( request, Seconds( 1 ) ).getStatus(),
					   Http::Response::Ok );
		}
		EXPECT_EQ( server.accepted.load(), policy == 1 ? 1u : 2u );
	}
}

UTEST( Http, compressedContentLengthRequiresValidatedDecoderCompletion ) {
	const std::string text( 70000, 'x' );
	for ( const auto mode : { System::Compression::MODE_GZIP, System::Compression::MODE_DEFLATE,
							  System::Compression::MODE_BROTLI } ) {
		System::IOStreamMemory input( text.data(), text.size() );
		System::IOStreamString encoded;
		if ( mode == System::Compression::MODE_BROTLI ) {
			// Fixed Brotli encoding of 70,000 'x' bytes; eepp includes no Brotli encoder.
			const unsigned char bytes[] = { 0x5b, 0x6f, 0x11, 0x81, 0x5f, 0x02, 0x2f,
											0x1e, 0x0b, 0x04, 0x52, 0x12, 0x02, 0x00 };
			encoded.write( reinterpret_cast<const char*>( bytes ), sizeof( bytes ) );
		} else {
			ASSERT_EQ( System::Compression::compress( encoded, input, mode ),
					   System::Compression::OK );
		}
		const char* encoding =
			mode == System::Compression::MODE_GZIP
				? "gzip"
				: ( mode == System::Compression::MODE_BROTLI ? "br" : "deflate" );
		HTTPTest::Server server( [&]( TcpSocket& socket, const std::string& request, unsigned ) {
			auto bytes = encoded.getStream();
			if ( request.find( "/truncated " ) != std::string::npos )
				bytes.pop_back();
			return HTTPTest::Server::respond(
				socket, bytes, std::string( "Content-Encoding: " ) + encoding + "\r\n" );
		} );
		ASSERT_TRUE( server.isListening() );
		Http::Pool pool;
		auto client = pool.get( server.uri() );
		for ( int i = 0; i < 2; ++i ) {
			auto good = client->sendRequest( Http::Request(), Seconds( 1 ) );
			EXPECT_EQ( good.getStatus(), Http::Response::Ok );
			EXPECT_TRUE( good.getBody() == text );
		}
		EXPECT_EQ( server.accepted.load(), 1u );
		auto truncated = client->sendRequest( Http::Request( "/truncated" ), Seconds( 1 ) );
		EXPECT_EQ( truncated.getStatus(), Http::Response::InvalidResponse );
		EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
				   Http::Response::Ok );
		EXPECT_EQ( server.accepted.load(), 2u );
	}
}

UTEST( Http, resumedDownloadReusesHeadConnectionAndRequestsRemainingRange ) {
	std::atomic<bool> sawRange{ false };
	HTTPTest::Server server( [&]( TcpSocket& socket, const std::string& request, unsigned ) {
		if ( request.find( "HEAD " ) == 0 ) {
			return HTTPTest::Server::send( socket, "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n"
												   "Accept-Ranges: bytes\r\n\r\n" );
		}
		sawRange = request.find( "range: bytes=3-\r\n" ) != std::string::npos;
		return HTTPTest::Server::send(
			socket, "HTTP/1.1 206 Partial Content\r\nContent-Length: 3\r\n\r\ndef" );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	System::IOStreamString output;
	output.write( "abc", 3 );
	Http::Request request;
	request.setContinue( true );
	auto response = pool.get( server.uri() )->downloadRequest( request, output, Seconds( 1 ) );
	EXPECT_EQ( response.getStatus(), Http::Response::PartialContent );
	EXPECT_TRUE( output.getStream() == "abcdef" );
	EXPECT_TRUE( sawRange.load() );
	EXPECT_EQ( server.accepted.load(), 1u );
}

UTEST( Http, destinationWriteFailureDiscardsConnection ) {
	struct ShortWriteStream : System::IOStreamString {
		ios_size write( const char*, ios_size size ) { return size - 1; }
	};
	HTTPTest::Server server( []( TcpSocket& socket, const std::string&, unsigned ) {
		return HTTPTest::Server::respond( socket );
	} );
	ASSERT_TRUE( server.isListening() );
	Http::Pool pool;
	auto client = pool.get( server.uri() );
	ShortWriteStream output;
	auto bad = client->downloadRequest( Http::Request(), output, Seconds( 1 ) );
	EXPECT_EQ( bad.getStatus(), Http::Response::InvalidResponse );
	EXPECT_EQ( client->sendRequest( Http::Request(), Seconds( 1 ) ).getStatus(),
			   Http::Response::Ok );
	EXPECT_EQ( server.accepted.load(), 2u );
}
