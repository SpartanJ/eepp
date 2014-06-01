#ifndef EE_NETWORKCSSLSOCKET_HPP
#define EE_NETWORKCSSLSOCKET_HPP

#include <eepp/network/ctcpsocket.hpp>

namespace EE { namespace Network { namespace SSL {

class cSSLSocketImpl;

class EE_API cSSLSocket : public cTcpSocket {
	public:
		static std::string CertificatesPath;

		static bool Init();
		
		static bool End();

		cSSLSocket( std::string hostname, bool validateCertificate, bool validateHostname );

		virtual ~cSSLSocket();

		Status Connect(const cIpAddress& remoteAddress, unsigned short remotePort, cTime timeout = cTime::Zero);

		void Disconnect();

		Status Send(const void* data, std::size_t size);

		Status Receive(void* data, std::size_t size, std::size_t& received);

		Status Send(cPacket& packet);

		Status Receive(cPacket& packet);
	protected:
		friend class cSSLSocketImpl;
		friend class cOpenSSLSocket;
		
		cSSLSocketImpl *		mImpl;
		std::string				mHostName;
		bool					mValidateCertificate;
		bool					mValidateHostname;
		
		Status TcpSend(const void* data, std::size_t size);

		Status TcpReceive(void* data, std::size_t size, std::size_t& received);
};

}}}

#endif
