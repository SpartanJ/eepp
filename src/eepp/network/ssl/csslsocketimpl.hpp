#ifndef EE_NETWORKCSSLSOCKETIMPL_HPP
#define EE_NETWORKCSSLSOCKETIMPL_HPP

#include <eepp/network/ssl/csslsocket.hpp>

namespace EE { namespace Network { namespace SSL {

class cSSLSocketImpl {
	public:
		cSSLSocketImpl( cSSLSocket * socket ) :
			mSSLSocket( socket )
		{}

		virtual ~cSSLSocketImpl() {}

		virtual cSocket::Status Connect(const cIpAddress& remoteAddress, unsigned short remotePort, cTime timeout = cTime::Zero) = 0;

		virtual void Disconnect() = 0;

		virtual cSocket::Status Send(const void* data, std::size_t size) = 0;

		virtual cSocket::Status Receive(void* data, std::size_t size, std::size_t& received) = 0;
	protected:
		cSSLSocket *	mSSLSocket;
};

}}}

#endif
