#ifndef EE_NETWORKCOPENLSSLSOCKET_HPP
#define EE_NETWORKCOPENLSSLSOCKET_HPP

#include <eepp/network/ssl/sslsocketimpl.hpp>

#ifdef EE_OPENSSL

extern "C" {
#if EE_PLATFORM == EE_PLATFORM_WIN
#undef X509_NAME
#undef X509_EXTENSIONS
#undef X509_CERT_PAIR
#undef PKCS7_ISSUER_AND_SERIAL
#undef OCSP_REQUEST
#undef OCSP_RESPONSE
#define NOCRYPT
#endif
#include <openssl/bio.h> // BIO objects for I/O
#include <openssl/ssl.h> // SSL and SSL_CTX for SSL connections
#include <openssl/err.h> // Error reporting
#include <openssl/x509v3.h>
}

namespace EE { namespace Network { namespace SSL {

class OpenSSLSocket : public SSLSocketImpl {
	public:
		static bool init();

		static bool end();

		OpenSSLSocket( SSLSocket * socket );

		~OpenSSLSocket();

		Socket::Status connect(const IpAddress& remoteAddress, unsigned short remotePort, Time timeout = Time::Zero);

		void disconnect();

		Socket::Status send(const void* data, std::size_t size);

		Socket::Status receive(void* data, std::size_t size, std::size_t& received);
	protected:
		SSL_CTX *		mCTX;
		::SSL *			mSSL;
		BIO *			mBIO;
		SSLSocket *	mSSLSocket;
		bool			mConnected;
		Socket::Status mStatus;
		int				mMaxCertChainDepth;

	private:
		static int certVerifyCb(X509_STORE_CTX *x509_ctx, void *arg);

		static bool matchHostname(const char *name, const char *hostname);

		static bool matchCommonName(const char *hostname, const X509 *server_cert);

		static bool matchSubjectAlternativeName(const char *hostname, const X509 *server_cert);

		void _print_error(int err);
};

}}}

#endif

#endif
