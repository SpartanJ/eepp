#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <cstdlib>
#include <cstring>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <thread>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

namespace {

// openssl s_server restricted to one protocol ("-tls1_2" or "-tls1_3"). Its -www status page
// reports "New, TLSv1.x, ..." for a full handshake and "Reused, TLSv1.x, ..." for a resumption.
class LocalTLSServer {
  public:
	explicit LocalTLSServer( const std::string& protocol ) {
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
							"-cert", mCertificate, "-key", key, "-www", "-quiet", protocol } ) )
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

// Sends a status-page request and returns its "New, ..." or "Reused, ..." line. It stops reading
// there: a socket that sees the server close discards its session.
std::string requestStatusLine( SSL::SSLSocket& socket ) {
	// connect() clears its timeouts, and a restored session's handshake runs inside send().
	socket.setSendTimeout( Seconds( 5 ) );
	socket.setReceiveTimeout( Seconds( 5 ) );
	const char request[] = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
	if ( socket.send( request, sizeof( request ) - 1 ) != Socket::Done )
		return {};
	std::string response;
	char buffer[1024];
	Clock clock;
	while ( response.size() < 65536 && clock.getElapsedTime() < Seconds( 5 ) ) {
		size_t received = 0;
		// Blocking reads must not surface TLS 1.3 NewSessionTicket processing as NotReady.
		if ( socket.receive( buffer, sizeof( buffer ), received ) != Socket::Done )
			break;
		response.append( buffer, received );
		size_t start = response.find( "\nNew, " );
		if ( start == std::string::npos )
			start = response.find( "\nReused, " );
		size_t end = start == std::string::npos ? start : response.find( '\n', start + 1 );
		if ( end != std::string::npos )
			return response.substr( start + 1, end - start - 1 );
	}
	return {};
}

bool requireServer( const LocalTLSServer& server ) {
	if ( server.available() )
		return true;
	const char* required = std::getenv( "EEPP_REQUIRE_TLS_TESTS" );
	return !( required && std::strcmp( required, "1" ) == 0 );
}

} // namespace

UTEST( TLSSecurity, HTTPSAcceptsTrustedPeerAndRejectsWrongHostAndUntrustedCertificate ) {
	if ( !SSL::SSLSocket::isSupported() )
		UTEST_SKIP( "TLS backend disabled" );
	for ( const char* protocol : { "-tls1_2", "-tls1_3" } ) {
		LocalTLSServer server( protocol );
		ASSERT_TRUE_MSG( requireServer( server ),
						 "Could not start the required loopback OpenSSL TLS server" );
		if ( !server.available() )
			UTEST_SKIP( "OpenSSL CLI with req -addext and s_server is required" );
		const std::string version = protocol[6] == '2' ? "TLSv1.2" : "TLSv1.3";
		TLSCertificateScope trust;
		{
			auto socket = TLSCertificateScope::socket( server.certificate(), "localhost" );
			ASSERT_EQ( socket->connect( IpAddress::LocalHost, server.port(), Seconds( 5 ) ),
					   Socket::Done );
			// The server accepts only `version`, and reports what was negotiated.
			EXPECT_STDSTREQ( requestStatusLine( *socket ).substr( 0, 12 ),
							 "New, " + version );
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
}

UTEST( TLSSecurity, RestoredSessionsResumeOnTLS12AndTLS13 ) {
	// FTPS data connections restore the control connection's session, and servers such as
	// vsftpd (require_ssl_reuse) reject data connections that do not resume it. TLS 1.3 can
	// only resume from a NewSessionTicket, which arrives after the handshake.
	if ( !SSL::SSLSocket::isSupported() )
		UTEST_SKIP( "TLS backend disabled" );
	for ( const char* protocol : { "-tls1_2", "-tls1_3" } ) {
		LocalTLSServer server( protocol );
		ASSERT_TRUE_MSG( requireServer( server ),
						 "Could not start the required loopback OpenSSL TLS server" );
		if ( !server.available() )
			UTEST_SKIP( "OpenSSL CLI with req -addext and s_server is required" );
		const std::string version = protocol[6] == '2' ? "TLSv1.2" : "TLSv1.3";
		TLSCertificateScope trust;
		auto control = TLSCertificateScope::socket( server.certificate(), "localhost" );
		ASSERT_EQ( control->connect( IpAddress::LocalHost, server.port(), Seconds( 5 ) ),
				   Socket::Done );
		EXPECT_STDSTREQ( requestStatusLine( *control ).substr( 0, 12 ),
						 "New, " + version );
		// s_server handles one connection at a time, so close this one; the socket keeps its
		// session for the restoring socket, as an FTPS control socket does.
		control->disconnect();
		std::unique_ptr<SSL::SSLSocket> data(
			SSL::SSLSocket::New( "localhost", true, true, control.get() ) );
		ASSERT_EQ( data->connect( IpAddress::LocalHost, server.port(), Seconds( 5 ) ),
				   Socket::Done );
		EXPECT_STDSTREQ( requestStatusLine( *data ).substr( 0, 15 ),
						 "Reused, " + version );
	}
}
