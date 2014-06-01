#include <eepp/network/ssl/csslsocket.hpp>
#include <eepp/network/ssl/csslsocketimpl.hpp>
#include <eepp/network/platform/platformimpl.hpp>

#ifdef EE_OPENSSL
#include <eepp/network/ssl/backend/openssl/copensslsocket.hpp>
#endif

namespace EE { namespace Network { namespace SSL {

static bool ssl_initialized = false;

std::string cSSLSocket::CertificatesPath = "";

bool cSSLSocket::Init() {
	bool ret = false;

	if ( !ssl_initialized ) {
		if ( CertificatesPath.empty() ) {
			#if EE_PLATFORM == EE_PLATFORM_LINUX
			CertificatesPath = "/etc/ssl/ca-bundle.pem";
			#endif
		}

		#ifdef EE_OPENSSL
		ret = cOpenSSLSocket::Init();
		#endif

		ssl_initialized = true;
	}

	return ret;
}

bool cSSLSocket::End() {
	bool ret = false;

	if ( ssl_initialized ) {
		#ifdef EE_OPENSSL
		ret = cOpenSSLSocket::End();
		#endif
		
		ssl_initialized = false;
	}

	return ret;
}

cSSLSocket::cSSLSocket( std::string hostname , bool validateCertificate, bool validateHostname ) :
#ifdef EE_OPENSSL
	mImpl( eeNew( cOpenSSLSocket, ( this ) ) ),
#else
	mImpl( NULL ),
#endif
	mHostName( hostname ),
	mValidateCertificate( validateCertificate ),
	mValidateHostname( validateHostname )
{
	Init();
}

cSSLSocket::~cSSLSocket() {
	eeSAFE_DELETE( mImpl );
}

cSocket::Status cSSLSocket::Connect( const cIpAddress& remoteAddress, unsigned short remotePort, cTime timeout ) {
	Status status = cSocket::Disconnected;
	
	if ( ( status = cTcpSocket::Connect( remoteAddress, remotePort, timeout ) ) == cSocket::Done ) {
		status = mImpl->Connect( remoteAddress, remotePort, timeout );
	}
	
	return status;
}

void cSSLSocket::Disconnect() {
	mImpl->Disconnect();
	cTcpSocket::Disconnect();
}

cSocket::Status cSSLSocket::Send(const void* data, std::size_t size) {
	return mImpl->Send( data, size );
}

cSocket::Status cSSLSocket::Receive(void* data, std::size_t size, std::size_t& received) {
	return mImpl->Receive( data, size, received );
}

cSocket::Status cSSLSocket::Send(cPacket& packet) {
	return cTcpSocket::Send( packet );
}

cSocket::Status cSSLSocket::Receive(cPacket& packet) {
	return cTcpSocket::Receive( packet );
}

cSocket::Status cSSLSocket::TcpSend(const void* data, std::size_t size) {
	return cTcpSocket::Send( data, size );
}

cSocket::Status cSSLSocket::TcpReceive(void* data, std::size_t size, std::size_t& received) {
	return cTcpSocket::Receive( data, size, received );
}

}}}
