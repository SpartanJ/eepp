#include "utest.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <eepp/network/http.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamstring.hpp>
#include <eepp/system/sys.hpp>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#endif

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

#if EE_PLATFORM != EE_PLATFORM_WIN
namespace {

struct FileDescriptorGuard {
	~FileDescriptorGuard() {
		for ( int fd : fds )
			::close( fd );
	}

	std::vector<int> fds;
};

} // namespace
#endif

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

	FileDescriptorGuard openFiles;
	openFiles.fds.reserve( FD_SETSIZE + 16 );
	while ( openFiles.fds.empty() || openFiles.fds.back() < FD_SETSIZE ) {
		int fd = ::open( "/dev/null", O_RDONLY );
		if ( fd < 0 )
			break;
		openFiles.fds.push_back( fd );
	}

	if ( openFiles.fds.empty() || openFiles.fds.back() < FD_SETSIZE )
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
