#ifndef EE_NETWORKCSSLSOCKET_HPP
#define EE_NETWORKCSSLSOCKET_HPP

#include <eepp/network/tcpsocket.hpp>

namespace EE { namespace Network { namespace SSL {

class SSLSocketImpl;

class EE_API SSLSocket : public TcpSocket {
	public:
		static std::string CertificatesPath;

		static bool Init();
		
		static bool End();

		/** @return True when the library was compiled with SSL support. */
		static bool IsSupported();

		SSLSocket( std::string hostname, bool validateCertificate, bool validateHostname );

		virtual ~SSLSocket();

		Status Connect(const IpAddress& remoteAddress, unsigned short remotePort, cTime timeout = cTime::Zero);

		void Disconnect();

		Status Send(const void* data, std::size_t size);

		Status Receive(void* data, std::size_t size, std::size_t& received);

		Status Send(Packet& packet);

		Status Receive(Packet& packet);
	protected:
		friend class SSLSocketImpl;
		friend class OpenSSLSocket;
		
		SSLSocketImpl *		mImpl;
		std::string				mHostName;
		bool					mValidateCertificate;
		bool					mValidateHostname;
		
		Status TcpSend(const void* data, std::size_t size);

		Status TcpReceive(void* data, std::size_t size, std::size_t& received);
};

}}}

#endif
