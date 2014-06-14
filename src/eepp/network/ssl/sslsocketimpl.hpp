#ifndef EE_NETWORKCSSLSOCKETIMPL_HPP
#define EE_NETWORKCSSLSOCKETIMPL_HPP

#include <eepp/network/ssl/sslsocket.hpp>

namespace EE { namespace Network { namespace SSL {

class SSLSocketImpl {
	public:
		SSLSocketImpl( SSLSocket * socket ) :
			mSSLSocket( socket )
		{}

		virtual ~SSLSocketImpl() {}

		virtual Socket::Status Connect(const IpAddress& remoteAddress, unsigned short remotePort, Time timeout = Time::Zero) = 0;

		virtual void Disconnect() = 0;

		virtual Socket::Status Send(const void* data, std::size_t size) = 0;

		virtual Socket::Status Receive(void* data, std::size_t size, std::size_t& received) = 0;
	protected:
		SSLSocket *	mSSLSocket;
};

}}}

#endif
