#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <cstdlib>
#include <cstring>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <thread>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

namespace {

class LocalTLSServer {
  public:
	LocalTLSServer() {
		const std::string openssl = Sys::which( "openssl" );
		if ( openssl.empty() )
			return;
		mCertificate = mDirectory.path + "server.pem";
		mOtherCertificate = mDirectory.path + "other.pem";
		const std::string key = mDirectory.path + "server.key";
		if ( !generate( openssl, mCertificate, key ) ||
			 !generate( openssl, mOtherCertificate, mDirectory.path + "other.key" ) )
			return;
		TcpListener reservation;
		if ( reservation.listen( Socket::AnyPort, IpAddress::LocalHost ) != Socket::Done )
			return;
		mPort = reservation.getLocalPort();
		reservation.close();
		if ( !mProcess.create(
				 openssl, { "s_server", "-accept", "127.0.0.1:" + String::toString( mPort ),
							"-cert", mCertificate, "-key", key, "-www", "-quiet", "-tls1_2" } ) )
			return;
		for ( int attempt = 0; attempt < 100; ++attempt ) {
			TcpSocket probe;
			if ( probe.connect( IpAddress::LocalHost, mPort, Milliseconds( 50 ) ) ==
				 Socket::Done ) {
				mAvailable = true;
				break;
			}
			std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
		}
	}

	~LocalTLSServer() { mProcess.kill(); }

	bool available() const { return mAvailable; }

	unsigned short port() const { return mPort; }

	const std::string& certificate() const { return mCertificate; }

	const std::string& otherCertificate() const { return mOtherCertificate; }

  private:
	static bool generate( const std::string& openssl, const std::string& certificate,
						  const std::string& key ) {
		Process process;
		if ( !process.create( openssl, { "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-sha256",
										 "-days", "1", "-subj", "/CN=localhost", "-addext",
										 "subjectAltName=DNS:localhost", "-keyout", key, "-out",
										 certificate } ) )
			return false;
		int code = -1;
		process.join( &code );
		return code == 0;
	}

	vfstest::TempDir mDirectory;
	Process mProcess;
	std::string mCertificate;
	std::string mOtherCertificate;
	unsigned short mPort{ 0 };
	bool mAvailable{ false };
};

class TLSCertificateScope {
  public:
	TLSCertificateScope() : mOriginal( SSL::SSLSocket::CertificatesPath ) { SSL::SSLSocket::end(); }

	~TLSCertificateScope() {
		SSL::SSLSocket::end();
		SSL::SSLSocket::CertificatesPath = std::move( mOriginal );
	}

	static std::unique_ptr<SSL::SSLSocket> socket( const std::string& certificate,
												   const std::string& hostname ) {
		SSL::SSLSocket::end();
		SSL::SSLSocket::CertificatesPath = certificate;
		return std::unique_ptr<SSL::SSLSocket>( SSL::SSLSocket::New( hostname, true, true ) );
	}

  private:
	std::string mOriginal;
};

} // namespace

UTEST( TLSSecurity, HTTPSAcceptsTrustedPeerAndRejectsWrongHostAndUntrustedCertificate ) {
	if ( !SSL::SSLSocket::isSupported() )
		UTEST_SKIP( "TLS backend disabled" );
	LocalTLSServer server;
	if ( !server.available() ) {
		const char* required = std::getenv( "EEPP_REQUIRE_TLS_TESTS" );
		ASSERT_FALSE_MSG( required && std::strcmp( required, "1" ) == 0,
						  "Could not start the required loopback OpenSSL TLS server" );
		UTEST_SKIP( "OpenSSL CLI with req -addext and s_server is required" );
	}
	TLSCertificateScope trust;
	{
		auto socket = TLSCertificateScope::socket( server.certificate(), "localhost" );
		ASSERT_EQ( socket->connect( IpAddress::LocalHost, server.port(), Seconds( 5 ) ),
				   Socket::Done );
		const char request[] = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
		ASSERT_EQ( socket->send( request, sizeof( request ) - 1 ), Socket::Done );
		char response[1024];
		size_t received = 0;
		ASSERT_EQ( socket->receive( response, sizeof( response ), received ), Socket::Done );
		ASSERT_GE( received, 5u );
		EXPECT_EQ( std::memcmp( response, "HTTP/", 5 ), 0 );
	}
	{
		auto socket = TLSCertificateScope::socket( server.certificate(), "wrong.invalid" );
		EXPECT_NE( socket->connect( IpAddress::LocalHost, server.port(), Seconds( 5 ) ),
				   Socket::Done );
	}
	{
		auto socket = TLSCertificateScope::socket( server.otherCertificate(), "localhost" );
		EXPECT_NE( socket->connect( IpAddress::LocalHost, server.port(), Seconds( 5 ) ),
				   Socket::Done );
	}
}
