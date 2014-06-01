#include <eepp/network/ssl/csslsocket.hpp>
#include <eepp/network/ssl/csslsocketimpl.hpp>
#include <eepp/system/filesystem.hpp>

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
			// Debian Systems
			if ( FileSystem::FileExists( "/etc/ssl/certs/ca-certificates.crt" ) ) {
				CertificatesPath = "/etc/ssl/certs/ca-certificates.crt";
			// SuSE Systems
			} else if ( FileSystem::FileExists( "/etc/ssl/ca-bundle.pem" ) ) {
				CertificatesPath = "/etc/ssl/ca-bundle.pem";
			// Redhat and Mandriva Systems
			} else if ( FileSystem::FileExists( "/etc/pki/tls/certs/ca-bundle.crt" ) ) {
				CertificatesPath = "/etc/pki/tls/certs/ca-bundle.crt";
			// older Redhat Systems
			} else if ( FileSystem::FileExists( "/usr/share/ssl/certs/ca-bundle.crt" ) ) {
				CertificatesPath = "/usr/share/ssl/certs/ca-bundle.crt";
			}
			#elif EE_PLATFORM == EE_PLATFORM_BSD
			// FreeBSD
			if ( FileSystem::FileExists( "/usr/local/share/certs/ca-root.crt" ) ) {
				CertificatesPath = "/usr/local/share/certs/ca-root.crt";
			// OpenBSD
			} else if ( FileSystem::FileExists( "/etc/ssl/cert.pem" ) ) {
				CertificatesPath = "/etc/ssl/cert.pem";
			}
			#elif EE_PLATFORM == EE_PLATFORM_HAIKU
			if ( FileSystem::FileExists( "/boot/common/data/ssl/cert.pem" ) ) {
				CertificatesPath = "/boot/common/data/ssl/cert.pem";
			}
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
