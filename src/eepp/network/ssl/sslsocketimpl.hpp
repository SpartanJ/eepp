#ifndef EE_NETWORKCSSLSOCKETIMPL_HPP
#define EE_NETWORKCSSLSOCKETIMPL_HPP

#include <eepp/network/ssl/sslsocket.hpp>

namespace EE { namespace Network { namespace SSL {

class EE_API SSLSocketImpl {
	public:
		SSLSocketImpl( SSLSocket * socket ) :
			mSSLSocket( socket )
		{}

		virtual ~SSLSocketImpl() {}

		virtual Socket::Status connect(const IpAddress& remoteAddress, unsigned short remotePort, Time timeout = Time::Zero) = 0;

		virtual void disconnect() = 0;

		virtual Socket::Status send(const void* data, std::size_t size) = 0;

		virtual Socket::Status receive(void* data, std::size_t size, std::size_t& received) = 0;
	protected:
		SSLSocket *	mSSLSocket;
};

}}}

#endif
