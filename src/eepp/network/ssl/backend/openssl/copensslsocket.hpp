#ifndef EE_NETWORKCOPENLSSLSOCKET_HPP
#define EE_NETWORKCOPENLSSLSOCKET_HPP

#include <eepp/network/ssl/csslsocketimpl.hpp>

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

class cOpenSSLSocket : public cSSLSocketImpl {
	public:
		static bool Init();

		static bool End();

		cOpenSSLSocket( cSSLSocket * socket );

		~cOpenSSLSocket();

		cSocket::Status Connect(const cIpAddress& remoteAddress, unsigned short remotePort, cTime timeout = cTime::Zero);

		void Disconnect();

		cSocket::Status Send(const void* data, std::size_t size);

		cSocket::Status Receive(void* data, std::size_t size, std::size_t& received);
	protected:
		SSL_CTX *		mCTX;
		::SSL *			mSSL;
		BIO *			mBIO;
		cSSLSocket *	mSSLSocket;
		bool			mConnected;
		cSocket::Status mStatus;
		int				mMaxCertChainDepth;

	private:
		static int CertVerifyCb(X509_STORE_CTX *x509_ctx, void *arg);

		static bool MatchHostname(const char *name, const char *hostname);

		static bool MatchCommonName(const char *hostname, const X509 *server_cert);

		static bool MatchSubjectAlternativeName(const char *hostname, const X509 *server_cert);

		void _print_error(int err);
};

}}}

#endif

#endif
