#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/ssl/sslsocketimpl.hpp>
#include <eepp/system/filesystem.hpp>

#ifdef EE_OPENSSL
#include <eepp/network/ssl/backend/openssl/opensslsocket.hpp>
#endif

namespace EE { namespace Network { namespace SSL {

static bool ssl_initialized = false;

std::string SSLSocket::CertificatesPath = "";

bool SSLSocket::Init() {
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
		ret = OpenSSLSocket::Init();
		#endif

		ssl_initialized = true;
	}

	return ret;
}

bool SSLSocket::End() {
	bool ret = false;

	if ( ssl_initialized ) {
		#ifdef EE_OPENSSL
		ret = OpenSSLSocket::End();
		#endif
		
		ssl_initialized = false;
	}

	return ret;
}

bool SSLSocket::IsSupported() {
#ifdef EE_SSL_SUPPORT
	return true;
#else
	return false;
#endif
}

SSLSocket::SSLSocket( std::string hostname , bool validateCertificate, bool validateHostname ) :
#ifdef EE_OPENSSL
	mImpl( eeNew( OpenSSLSocket, ( this ) ) ),
#else
	mImpl( NULL ),
#endif
	mHostName( hostname ),
	mValidateCertificate( validateCertificate ),
	mValidateHostname( validateHostname )
{
	Init();
}

SSLSocket::~SSLSocket() {
	eeSAFE_DELETE( mImpl );
}

Socket::Status SSLSocket::Connect( const IpAddress& remoteAddress, unsigned short remotePort, cTime timeout ) {
	Status status = Socket::Disconnected;
	
	if ( ( status = TcpSocket::Connect( remoteAddress, remotePort, timeout ) ) == Socket::Done ) {
		status = mImpl->Connect( remoteAddress, remotePort, timeout );
	}
	
	return status;
}

void SSLSocket::Disconnect() {
	mImpl->Disconnect();
	TcpSocket::Disconnect();
}

Socket::Status SSLSocket::Send(const void* data, std::size_t size) {
	return mImpl->Send( data, size );
}

Socket::Status SSLSocket::Receive(void* data, std::size_t size, std::size_t& received) {
	return mImpl->Receive( data, size, received );
}

Socket::Status SSLSocket::Send(Packet& packet) {
	return TcpSocket::Send( packet );
}

Socket::Status SSLSocket::Receive(Packet& packet) {
	return TcpSocket::Receive( packet );
}

Socket::Status SSLSocket::TcpSend(const void* data, std::size_t size) {
	return TcpSocket::Send( data, size );
}

Socket::Status SSLSocket::TcpReceive(void* data, std::size_t size, std::size_t& received) {
	return TcpSocket::Receive( data, size, received );
}

}}}
