#ifndef EE_NETWORKCMBEDTLSSOCKET_HPP
#define EE_NETWORKCMBEDTLSSOCKET_HPP

#include <eepp/network/ssl/sslsocketimpl.hpp>

#ifdef EE_MBEDTLS

#include <mbedtls/config.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/net.h>
#include <mbedtls/ssl.h>

namespace EE { namespace Network { namespace SSL {

class EE_API MbedTLSSocket : public SSLSocketImpl {
	public:
		static bool init();

		static bool end();

		MbedTLSSocket( SSLSocket * socket );

		~MbedTLSSocket();

		Socket::Status connect(const IpAddress& remoteAddress, unsigned short remotePort, Time timeout = Time::Zero);

		void disconnect();

		Socket::Status send(const void* data, std::size_t size);

		Socket::Status send(const void* data, std::size_t size, std::size_t& sent );

		Socket::Status receive(void* data, std::size_t size, std::size_t& received);
	protected:
		static mbedtls_x509_crt sCACert;
		mbedtls_entropy_context mEntropy;
		mbedtls_ctr_drbg_context mCtrDrbg;
		mbedtls_ssl_context mSSLContext;
		mbedtls_ssl_config mSSLConfig;
		bool mConnected;
		bool mSessionOwner;
		Socket::Status mStatus;
		mbedtls_ssl_session* mSSLSession;
		static int bio_send(void * ctx, const unsigned char * buf, size_t len);
		static int bio_recv(void * ctx, unsigned char * buf, size_t len);
};

}}}

#endif

#endif
