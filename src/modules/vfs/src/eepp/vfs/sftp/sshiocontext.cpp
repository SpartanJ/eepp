#include "sshiocontext.hpp"
#include "sshconnection.hpp"
#include <algorithm>
#include <libssh2.h>
#include <mutex>

namespace EE { namespace VFS { namespace sftp {

namespace {

void libssh2GlobalInit() {
	static std::once_flag once;
	std::call_once( once, [] {
		SSHSocket::init();
		libssh2_init( 0 );
	} );
}

} // namespace

SSHIOContext::SSHIOContext() {
	libssh2GlobalInit();
}

SSHIOContext::~SSHIOContext() {
	stop();
}

void SSHIOContext::start() {
	if ( mThread.joinable() )
		return;
	mThread = std::thread( [this] { loop(); } );
	mThreadId = mThread.get_id();
}

void SSHIOContext::stop() {
	if ( !mThread.joinable() )
		return;
	mStopping = true;
	mWake.signal();
	mThread.join();
	std::vector<Command> commands;
	{
		std::lock_guard<std::mutex> lock( mMutex );
		commands.swap( mCommands );
	}
}

void SSHIOContext::post( Command command ) {
	{
		std::lock_guard<std::mutex> lock( mMutex );
		if ( mStopping.load( std::memory_order_acquire ) )
			return;
		mCommands.emplace_back( std::move( command ) );
	}
	mWake.signal();
}

void SSHIOContext::addConnection( std::shared_ptr<SSHConnection> connection ) {
	mConnections.push_back( std::move( connection ) );
}

void SSHIOContext::closeConnections() {
	post( [this] {
		for ( auto& connection : mConnections ) {
			connection->fail(
				VFSError( VFSErrorCode::ConnectionLost, URI(), "Connection closed" ) );
		}
	} );
}

void SSHIOContext::loop() {
	std::vector<Command> commands;
	std::vector<PollFd> fds;
	while ( !mStopping.load( std::memory_order_acquire ) ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			commands.swap( mCommands );
		}
		for ( auto& command : commands )
			command();
		commands.clear();

		for ( size_t i = 0; i < mConnections.size(); ++i )
			mConnections[i]->drive();
		// Drop finished connections; their owners (pool, handles) may still reference them.
		mConnections.erase( std::remove_if( mConnections.begin(), mConnections.end(),
											[]( const std::shared_ptr<SSHConnection>& c ) {
												if ( c->isUsable() || c->hasQueuedOperations() )
													return false;
												// A keepalive/idle failure can happen at the end of
												// drive(). Notify any pumps before reclaiming their
												// channels.
												c->drive();
												c->releaseSession();
												return true;
											} ),
							mConnections.end() );

		fds.clear();
		PollFd wake{};
		wake.fd = mWake.pollHandle();
		wake.events = POLLIN;
		fds.push_back( wake );
		int timeout = -1;
		for ( const auto& connection : mConnections ) {
			connection->collectPoll( fds );
			int next = connection->nextTimeoutMs();
			if ( next >= 0 && ( timeout < 0 || next < timeout ) )
				timeout = next;
		}
		SSHSocket::poll( fds.data(), fds.size(), timeout );
		mWake.drain();
	}
	// Consumers can retain failed connections after stop() returns. Reclaim their sessions
	// here, so releasing those references later on the UI thread performs no protocol work.
	for ( auto& connection : mConnections )
		connection->fail( VFSError( VFSErrorCode::ConnectionLost, URI(), "SSH layer shut down" ) );
	for ( auto& connection : mConnections ) {
		connection->drive();
		connection->releaseSession();
	}
	mConnections.clear();
}

}}} // namespace EE::VFS::sftp
