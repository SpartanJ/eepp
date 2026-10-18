#include "sshchanneltransport.hpp"
#include "sshconnection.hpp"
#include <cerrno>
#include <eepp/core/string.hpp>

namespace EE { namespace VFS { namespace sftp {

SSHChannelTransport::SSHChannelTransport( std::shared_ptr<SSHConnection> parent, std::string host,
										  Uint16 port ) :
	mParent( std::move( parent ) ), mHost( std::move( host ) ), mPort( port ) {
	// The child's traffic is invisible to the parent's queue: keep the parent from closing as
	// idle while the child lives.
	mParent->addUser();
}

SSHChannelTransport::~SSHChannelTransport() {
	mParent->removeUser();
	// The channel belongs to the parent session; libssh2_session_free() of the parent releases
	// it. Freeing it here could race with the parent's I/O thread usage.
}

SocketHandle SSHChannelTransport::pollSocket() const {
	return mParent->pollSocket();
}

bool SSHChannelTransport::alive() const {
	return mParent->isUsable();
}

short SSHChannelTransport::pollEvents( short directions ) const {
	// Data for the child arrives through the parent's socket.
	return static_cast<short>( directions | mParent->pollEvents() | POLLIN );
}

libssh2_socket_t SSHChannelTransport::handshakeSocket() const {
	return static_cast<libssh2_socket_t>( mParent->pollSocket() );
}

Step SSHChannelTransport::open( VFSError& error ) {
	bool again = false;
	mChannel = mParent->openDirectTcpip( mHost, mPort, again );
	if ( mChannel )
		return Step::Done;
	if ( again )
		return Step::Again;
	error =
		mParent->lastError( URI(), String::format( "ProxyJump: could not open a channel to %s:%u",
												   mHost, static_cast<unsigned int>( mPort ) )
									   .c_str() );
	return Step::Done;
}

void SSHChannelTransport::install( LIBSSH2_SESSION* session ) {
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_SEND,
								   reinterpret_cast<libssh2_cb_generic*>( &sendCallback ) );
	libssh2_session_callback_set2( session, LIBSSH2_CALLBACK_RECV,
								   reinterpret_cast<libssh2_cb_generic*>( &recvCallback ) );
}

ssize_t SSHChannelTransport::sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
										   void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<SSHChannelTransport*>( connection->transport() );
	ssize_t rc =
		libssh2_channel_write( transport->mChannel, static_cast<const char*>( buffer ), length );
	if ( rc == LIBSSH2_ERROR_EAGAIN )
		return -EAGAIN;
	if ( rc < 0 )
		return -ECONNRESET;
	return rc;
}

ssize_t SSHChannelTransport::recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
										   void** abstract ) {
	auto* connection = static_cast<SSHConnection*>( *abstract );
	auto* transport = static_cast<SSHChannelTransport*>( connection->transport() );
	ssize_t rc = libssh2_channel_read( transport->mChannel, static_cast<char*>( buffer ), length );
	if ( rc == LIBSSH2_ERROR_EAGAIN )
		return -EAGAIN;
	if ( rc < 0 )
		return -ECONNRESET;
	if ( rc == 0 && !libssh2_channel_eof( transport->mChannel ) )
		return -EAGAIN;
	return rc;
}

}}} // namespace EE::VFS::sftp
