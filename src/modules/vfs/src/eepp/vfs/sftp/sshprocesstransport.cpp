#include "sshprocesstransport.hpp"
#include "sshconnection.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <eepp/system/process.hpp>
#include <vector>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <sys/socket.h>
#endif

namespace EE { namespace VFS { namespace sftp {

SSHProcessTransport::SSHProcessTransport( std::shared_ptr<SSHIOContext> context,
										  const std::string& command ) :
	mContext( std::move( context ) ), mProcess( std::make_unique<System::Process>() ) {
	mPlaceholder = ::socket( AF_INET, SOCK_STREAM, 0 );
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::string shell = "cmd";
	std::vector<std::string> args{ "/c", command };
#else
	const char* envShell = std::getenv( "SHELL" );
	std::string shell = envShell && *envShell ? envShell : "/bin/sh";
	std::vector<std::string> args{ "-c", command };
#endif
	if ( !mProcess->create( shell, args,
							System::Process::getDefaultOptions() |
								System::Process::Options::EnableAsync ) ) {
		mProcess.reset();
		return;
	}
	mProcess->startAsyncRead( [this]( const char* bytes, size_t n ) {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			mBuffer.append( bytes, n );
		}
		mContext->wake();
	} );
}

SSHProcessTransport::~SSHProcessTransport() {
	if ( mProcess ) {
		mProcess->kill();
		mProcess.reset();
	}
	SSHSocket::close( mPlaceholder );
}

bool SSHProcessTransport::started() const {
	return nullptr != mProcess;
}

bool SSHProcessTransport::alive() const {
	if ( !mProcess )
		return false;
	std::lock_guard<std::mutex> lock( mMutex );
	return !mBuffer.empty() || mProcess->isAlive();
}

libssh2_socket_t SSHProcessTransport::handshakeSocket() const {
	// libssh2 only hands this value to the callbacks below.
	return static_cast<libssh2_socket_t>( mPlaceholder );
}

void SSHProcessTransport::install( LIBSSH2_SESSION* session ) {
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_SEND,
								   reinterpret_cast<libssh2_cb_generic*>( &sendCallback ) );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_RECV,
								   reinterpret_cast<libssh2_cb_generic*>( &recvCallback ) );
}

ssize_t SSHProcessTransport::sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
										   void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<SSHProcessTransport*>( connection->transport() );
	if ( !transport->mProcess || !transport->mProcess->isAlive() )
		return -ECONNRESET;
	size_t written = transport->mProcess->write( static_cast<const char*>( buffer ), length );
	return written == 0 ? -EAGAIN : static_cast<ssize_t>( written );
}

ssize_t SSHProcessTransport::recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
										   void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<SSHProcessTransport*>( connection->transport() );
	std::lock_guard<std::mutex> lock( transport->mMutex );
	if ( transport->mBuffer.empty() ) {
		if ( !transport->mProcess || !transport->mProcess->isAlive() )
			return 0;
		return -EAGAIN;
	}
	size_t count = std::min( length, transport->mBuffer.size() );
	std::memcpy( buffer, transport->mBuffer.data(), count );
	transport->mBuffer.erase( 0, count );
	return static_cast<ssize_t>( count );
}

}}} // namespace EE::VFS::sftp
