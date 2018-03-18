#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/ssl/sslsocketimpl.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/lock.hpp>

#ifdef EE_OPENSSL
#include <eepp/network/ssl/backend/openssl/opensslsocket.hpp>
#endif

#ifdef EE_MBEDTLS
#include <eepp/network/ssl/backend/mbedtls/mbedtlssocket.hpp>
#endif

using namespace EE::System;

namespace EE { namespace Network { namespace SSL {

static bool ssl_initialized = false;

std::string SSLSocket::CertificatesPath	= "";
static Mutex sMutex;

bool SSLSocket::init() {
	Lock l( sMutex );

	bool ret = false;

	if ( !ssl_initialized ) {
		if ( CertificatesPath.empty() ) {
			#if EE_PLATFORM == EE_PLATFORM_LINUX
			// Debian Systems
			if ( FileSystem::fileExists( "/etc/ssl/certs/ca-certificates.crt" ) ) {
				CertificatesPath = "/etc/ssl/certs/ca-certificates.crt";
			// SuSE Systems
			} else if ( FileSystem::fileExists( "/etc/ssl/ca-bundle.pem" ) ) {
				CertificatesPath = "/etc/ssl/ca-bundle.pem";
			// Redhat and Mandriva Systems
			} else if ( FileSystem::fileExists( "/etc/pki/tls/certs/ca-bundle.crt" ) ) {
				CertificatesPath = "/etc/pki/tls/certs/ca-bundle.crt";
			// older Redhat Systems
			} else if ( FileSystem::fileExists( "/usr/share/ssl/certs/ca-bundle.crt" ) ) {
				CertificatesPath = "/usr/share/ssl/certs/ca-bundle.crt";
			}
			#elif EE_PLATFORM == EE_PLATFORM_BSD
			// FreeBSD
			if ( FileSystem::fileExists( "/usr/local/share/certs/ca-root.crt" ) ) {
				CertificatesPath = "/usr/local/share/certs/ca-root.crt";
			// OpenBSD
			} else if ( FileSystem::fileExists( "/etc/ssl/cert.pem" ) ) {
				CertificatesPath = "/etc/ssl/cert.pem";
			}
			#elif EE_PLATFORM == EE_PLATFORM_HAIKU
			if ( FileSystem::fileExists( "/boot/common/data/ssl/cert.pem" ) ) {
				CertificatesPath = "/boot/common/data/ssl/cert.pem";
			}
			#endif

			if ( CertificatesPath.empty() ) {
				CertificatesPath = "assets/ca-bundle.pem";
			}
		}

		#ifdef EE_OPENSSL
		ret = OpenSSLSocket::init();
		#endif

		#ifdef EE_MBEDTLS
		ret = MbedTLSSocket::init();
		#endif

		ssl_initialized = true;
	}

	return ret;
}

bool SSLSocket::end() {
	Lock l( sMutex );

	bool ret = false;

	if ( ssl_initialized ) {
		#ifdef EE_OPENSSL
		ret = OpenSSLSocket::end();
		#endif

		#ifdef EE_MBEDTLS
		ret = MbedTLSSocket::end();
		#endif

		ssl_initialized = false;
	}

	return ret;
}

bool SSLSocket::isSupported() {
#ifdef EE_SSL_SUPPORT
	return true;
#else
	return false;
#endif
}

SSLSocket::SSLSocket( std::string hostname , bool validateCertificate, bool validateHostname ) :
#ifdef EE_MBEDTLS
	mImpl( eeNew( MbedTLSSocket, ( this ) ) ),
#elif defined( EE_OPENSSL )
	mImpl( eeNew( OpenSSLSocket, ( this ) ) ),
#else
	mImpl( NULL ),
#endif
	mHostName( hostname ),
	mValidateCertificate( validateCertificate ),
	mValidateHostname( validateHostname )
{
	init();
}

SSLSocket::~SSLSocket() {
	eeSAFE_DELETE( mImpl );
}

Socket::Status SSLSocket::connect( const IpAddress& remoteAddress, unsigned short remotePort, Time timeout ) {
	Status status = Socket::Disconnected;
	
	if ( ( status = TcpSocket::connect( remoteAddress, remotePort, timeout ) ) == Socket::Done ) {
		status = mImpl->connect( remoteAddress, remotePort, timeout );
	}
	
	return status;
}

void SSLSocket::disconnect() {
	mImpl->disconnect();
	TcpSocket::disconnect();
}

Socket::Status SSLSocket::send(const void* data, std::size_t size) {
	return mImpl->send( data, size );
}

Socket::Status SSLSocket::receive(void* data, std::size_t size, std::size_t& received) {
	return mImpl->receive( data, size, received );
}

Socket::Status SSLSocket::send(Packet& packet) {
	return TcpSocket::send( packet );
}

Socket::Status SSLSocket::receive(Packet& packet) {
	return TcpSocket::receive( packet );
}

Socket::Status SSLSocket::tcp_receive(void * data, std::size_t size, std::size_t & received) {
	return TcpSocket::receive( data, size, received );
}

Socket::Status SSLSocket::tcp_send(const void * data, std::size_t size, std::size_t & sent) {
	return TcpSocket::send( data, size, sent );
}

}}}
