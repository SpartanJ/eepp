#include <eepp/network/ssl/backend/mbedtls/mbedtlssocket.hpp>

#ifdef EE_MBEDTLS

#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <mbedtls/error.h>

namespace EE { namespace Network { namespace SSL {

mbedtls_x509_crt MbedTLSSocket::sCACert;

bool MbedTLSSocket::init() {
	mbedtls_x509_crt_init(&sCACert);

	//! Load the certificates and config
	SafeDataPointer data;

	if ( FileSystem::fileExists( SSLSocket::CertificatesPath ) ) {
		FileSystem::fileGet( SSLSocket::CertificatesPath, data );
	} else if ( PackManager::instance()->isFallbackToPacksActive() ) {
		std::string tPath( SSLSocket::CertificatesPath );

		Pack * tPack = PackManager::instance()->exists( tPath );

		if ( NULL != tPack ) {
			tPack->extractFileToMemory( tPath, data );
		}
	}

	if ( data.size > 0 ) {
		SafeDataPointer dataZeroEnded( data.size + 1 );
		memcpy( dataZeroEnded.data, data.data, data.size );
		dataZeroEnded.data[ data.size ] = '\0';

		int err = mbedtls_x509_crt_parse( &sCACert, (const unsigned char*)dataZeroEnded.data, dataZeroEnded.size );

		if ( err != 0 ) {
			char errStr[ 1024 ];
			mbedtls_strerror( err, errStr, eeARRAY_SIZE( errStr ) );
			eePRINTL("Error parsing some certificates \"%s\": %d. Description: %s", SSLSocket::CertificatesPath.c_str(),  err, errStr );
		}
	} else {
		eePRINTL( "No certificate provided for TLS/SSL requests" );
	}

	return true;
}

bool MbedTLSSocket::end() {
	mbedtls_x509_crt_free(&sCACert);

	return true;
}

MbedTLSSocket::MbedTLSSocket( SSLSocket * socket ) :
	SSLSocketImpl( socket ),
	mConnected( false ),
	mStatus( Socket::Disconnected )
{
	mSSLSocket = socket;
}

MbedTLSSocket::~MbedTLSSocket() {
	disconnect();
}

int MbedTLSSocket::bio_send(void *ctx, const unsigned char *buf, size_t len) {

	if (buf == NULL || len <= 0) return 0;

	MbedTLSSocket *sp = (MbedTLSSocket *)ctx;

	size_t sent;
	Socket::Status err = sp->mSSLSocket->tcp_send((const void*)buf, len, sent);

	if (err != Socket::Done) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if (sent == 0) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}
	return sent;
}

int MbedTLSSocket::bio_recv(void *ctx, unsigned char *buf, size_t len) {

	if (buf == NULL || len <= 0) return 0;

	MbedTLSSocket *sp = (MbedTLSSocket *)ctx;

	size_t got;
	Socket::Status err = sp->mSSLSocket->tcp_receive(buf, len, got);

	if (err != Socket::Done) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if (got == 0) {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	return got;
}

Socket::Status MbedTLSSocket::connect( const IpAddress& remoteAddress, unsigned short remotePort, Time timeout ) {
	if ( mConnected ) {
		disconnect();
	}

	int ret = 0;
	int authmode = mSSLSocket->mValidateCertificate ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE;

	mbedtls_ssl_init(&mSSLContext);
	mbedtls_ssl_config_init(&mSSLConfig);
	mbedtls_ctr_drbg_init(&mCtrDrbg);
	mbedtls_entropy_init(&mEntropy);
	
	ret = mbedtls_ctr_drbg_seed(&mCtrDrbg, mbedtls_entropy_func, &mEntropy, NULL, 0);

	if (ret != 0) {
		eePRINTL(" failed\n  ! mbedtls_ctr_drbg_seed returned an error: %d", ret );
		return Socket::Error;
	}

	mbedtls_ssl_config_defaults(&mSSLConfig, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);

	mbedtls_ssl_conf_authmode(&mSSLConfig, authmode);
	mbedtls_ssl_conf_ca_chain(&mSSLConfig, &sCACert, NULL);
	mbedtls_ssl_conf_rng(&mSSLConfig, mbedtls_ctr_drbg_random, &mCtrDrbg);
	mbedtls_ssl_setup(&mSSLContext, &mSSLConfig);
	mbedtls_ssl_set_hostname(&mSSLContext, mSSLSocket->mHostName.c_str() );
	mbedtls_ssl_set_bio(&mSSLContext, this, bio_send, bio_recv, NULL);

	while ((ret = mbedtls_ssl_handshake(&mSSLContext)) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mStatus = Socket::Error;
			return mStatus;
		}
	}

	mConnected = true;
	mStatus = Socket::Done;

	return mStatus;
}

void MbedTLSSocket::disconnect() {
	if (!mConnected)
		return;

	mbedtls_ssl_free(&mSSLContext);
	mbedtls_ssl_config_free(&mSSLConfig);
	mbedtls_ctr_drbg_free(&mCtrDrbg);
	mbedtls_entropy_free(&mEntropy);
	mStatus		= Socket::Disconnected;
}

Socket::Status MbedTLSSocket::send( const void * data, std::size_t size ) {
	Socket::Status err;
	size_t sent = 0;
	const char * ptr = (const char*)data;

	while (size > 0) {
		err = send(ptr, size, sent);

		if (err != Socket::Done) {
			return err;
		}

		ptr += sent;
		size -= sent;
	}

	return Socket::Done;
}

Socket::Status MbedTLSSocket::send(const void * data, std::size_t size, std::size_t & sent) {
	sent = 0;

	if (size == 0)
		return Socket::Done;

	int ret = mbedtls_ssl_write(&mSSLContext, (const unsigned char*)data, size);

	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		ret = 0; // non blocking io
	} else if (ret <= 0) {
		disconnect();
		return Socket::Disconnected;
	}

	sent = ret;
	return Socket::Done;
}

Socket::Status MbedTLSSocket::receive( void * data, std::size_t size, std::size_t& received ) {
	received = 0;

	int ret = mbedtls_ssl_read(&mSSLContext, (unsigned char*)data, size);
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		ret = 0; // non blocking io
	} else if (ret <= 0) {
		disconnect();
		return Socket::Disconnected;
	}

	received = ret;
	return Socket::Done;
}

}}}

#endif
