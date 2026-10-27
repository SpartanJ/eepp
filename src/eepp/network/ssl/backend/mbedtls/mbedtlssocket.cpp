#include <eepp/config.hpp>

#ifdef EE_MBEDTLS

#include <eepp/network/ssl/backend/mbedtls/mbedtlssocket.hpp>

#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packregistry.hpp>
#include <mbedtls/error.h>
#include <psa/crypto.h>

namespace EE { namespace Network { namespace SSL {

mbedtls_x509_crt MbedTLSSocket::sCACert;

bool MbedTLSSocket::init() {
	// TLS 1.3 handshakes run on PSA crypto, which must be initialized first.
	psa_status_t psaStatus = psa_crypto_init();
	if ( psaStatus != PSA_SUCCESS )
		Log::error( "psa_crypto_init failed: %d. TLS 1.3 connections will fail.", (int)psaStatus );

	mbedtls_x509_crt_init( &sCACert );

	//! Load the certificates and config
	ScopedBuffer data;

	if ( FileSystem::fileExists( SSLSocket::CertificatesPath ) ) {
		FileSystem::fileGet( SSLSocket::CertificatesPath, data );
	} else if ( PackRegistry::instance()->isFallbackToPacksActive() ) {
		std::string tPath( SSLSocket::CertificatesPath );

		Pack* tPack = PackRegistry::instance()->exists( tPath );

		if ( NULL != tPack ) {
			tPack->extractFileToMemory( tPath, data );
		}
	}

	if ( data.length() > 0 ) {
		ScopedBuffer dataZeroEnded( data.length() + 1 );
		memcpy( dataZeroEnded.get(), data.get(), data.length() );
		dataZeroEnded[data.length()] = '\0';

		int err = mbedtls_x509_crt_parse( &sCACert, (const unsigned char*)dataZeroEnded.get(),
										  dataZeroEnded.length() );

		if ( err != 0 ) {
			char errStr[1024];
			mbedtls_strerror( err, errStr, eeARRAY_SIZE( errStr ) );
			Log::error( "Error parsing some certificates \"%s\": %d. Description: %s",
						SSLSocket::CertificatesPath.c_str(), err, errStr );
		}
	} else {
		Log::warning( "No certificate provided for TLS/SSL requests" );
	}

	return true;
}

bool MbedTLSSocket::end() {
	mbedtls_x509_crt_free( &sCACert );
	// SSLSocket defers end() until no socket is alive, so no handshake still uses PSA.
	mbedtls_psa_crypto_free();

	return true;
}

void MbedTLSSocket::saveSession() {
	mbedtls_ssl_session* session = (mbedtls_ssl_session*)eeMalloc( sizeof( mbedtls_ssl_session ) );
	if ( !session )
		return;

	mbedtls_ssl_session_init( session );
	if ( mbedtls_ssl_get_session( &mSSLContext, session ) != 0 ) {
		mbedtls_ssl_session_free( session );
		eeFree( session );
		return;
	}

	Lock l( mSessionMutex );
	if ( mSessionOwner && NULL != mSSLSession ) {
		mbedtls_ssl_session_free( mSSLSession );
		eeFree( mSSLSession );
	}
	mSSLSession = session;
	mSessionOwner = true;
}

void MbedTLSSocket::releaseSession() {
	Lock l( mSessionMutex );
	if ( mSessionOwner && NULL != mSSLSession ) {
		mbedtls_ssl_session_free( mSSLSession );
		eeSAFE_FREE( mSSLSession );
	}
	mSessionOwner = false;
}

MbedTLSSocket::MbedTLSSocket( SSLSocket* socket ) :
	SSLSocketImpl( socket ),
	mInitialized( false ),
	mConnected( false ),
	mSessionOwner( false ),
	mStatus( Socket::Disconnected ),
	mSSLSession( NULL ) {
	mSSLSocket = socket;
}

MbedTLSSocket::~MbedTLSSocket() {
	disconnect();
	releaseSession();
}

int MbedTLSSocket::bio_send( void* ctx, const unsigned char* buf, size_t len ) {

	if ( buf == NULL || len <= 0 )
		return 0;

	MbedTLSSocket* sp = (MbedTLSSocket*)ctx;

	size_t sent;
	Socket::Status err = sp->mSSLSocket->tcpSend( (const void*)buf, len, sent );

	if ( err != Socket::Done ) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if ( sent == 0 ) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}
	return sent;
}

int MbedTLSSocket::bio_recv( void* ctx, unsigned char* buf, size_t len ) {

	if ( buf == NULL || len <= 0 )
		return 0;

	MbedTLSSocket* sp = (MbedTLSSocket*)ctx;

	size_t got;
	Socket::Status err = sp->mSSLSocket->tcpReceive( buf, len, got );

	if ( err == Socket::NotReady )
		return MBEDTLS_ERR_SSL_WANT_READ;
	if ( err == Socket::Disconnected )
		return 0;
	if ( err != Socket::Done )
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	if ( got == 0 )
		return MBEDTLS_ERR_SSL_WANT_READ;
	return got;
}

Socket::Status MbedTLSSocket::connect( const IpAddress& /*remoteAddress*/,
									   unsigned short /*remotePort*/, Time timeout ) {
	// A previous handshake may have initialized TLS state without ever reaching Connected.
	// Always release it, and the previous connection's session, before a new handshake.
	disconnect();
	releaseSession();

	bool isBlocking = mSSLSocket->isBlocking();
	if ( isBlocking && timeout != Time::Zero ) {
		mSSLSocket->setReceiveTimeout( timeout );
		mSSLSocket->setSendTimeout( timeout );
	}

	Clock clock;
	int ret = 0;
	int authmode =
		mSSLSocket->mValidateCertificate ? MBEDTLS_SSL_VERIFY_REQUIRED : MBEDTLS_SSL_VERIFY_NONE;

	mbedtls_ssl_init( &mSSLContext );
	mbedtls_ssl_config_init( &mSSLConfig );
	mbedtls_ctr_drbg_init( &mCtrDrbg );
	mbedtls_entropy_init( &mEntropy );
	mInitialized = true;

	ret = mbedtls_ctr_drbg_seed( &mCtrDrbg, mbedtls_entropy_func, &mEntropy, NULL, 0 );

	if ( ret != 0 ) {
		Log::error( " failed\n  ! mbedtls_ctr_drbg_seed returned an error: %d", ret );
		disconnect();
		mStatus = Socket::Error;
		return mStatus;
	}

	mbedtls_ssl_config_defaults( &mSSLConfig, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
								 MBEDTLS_SSL_PRESET_DEFAULT );

	mbedtls_ssl_conf_authmode( &mSSLConfig, authmode );
	mbedtls_ssl_conf_ca_chain( &mSSLConfig, &sCACert, NULL );
	mbedtls_ssl_conf_rng( &mSSLConfig, mbedtls_ctr_drbg_random, &mCtrDrbg );
	// TLS 1.2 resumes by session ID; TLS 1.3 can only resume from a NewSessionTicket, which the
	// server sends after the handshake. Ask to be told about tickets so saveSession() can keep the
	// latest one for sockets that restore this session (FTPS data connections).
	mbedtls_ssl_conf_session_tickets( &mSSLConfig, MBEDTLS_SSL_SESSION_TICKETS_DISABLED );
#if defined( MBEDTLS_SSL_PROTO_TLS1_3 ) && defined( MBEDTLS_SSL_SESSION_TICKETS )
	mbedtls_ssl_conf_tls13_enable_signal_new_session_tickets(
		&mSSLConfig, MBEDTLS_SSL_TLS1_3_SIGNAL_NEW_SESSION_TICKETS_ENABLED );
#endif
	mbedtls_ssl_setup( &mSSLContext, &mSSLConfig );
	mbedtls_ssl_set_hostname( &mSSLContext, mSSLSocket->mHostName.c_str() );
	mbedtls_ssl_set_bio( &mSSLContext, this, bio_send, bio_recv, NULL );

	if ( mSSLSocket->mRestoreSession == NULL ) {
		while ( ( ret = mbedtls_ssl_handshake( &mSSLContext ) ) != 0 ) {
			if ( ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET ) {
				saveSession();
				continue;
			} else if ( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
				mStatus = Socket::Error;
				break;
			} else if ( timeout != Time::Zero && clock.getElapsedTime() >= timeout ) {
				mStatus = Socket::Error;
				break;
			}

			if ( !isBlocking )
				Sys::sleep( Milliseconds( 1 ) );
		}

		if ( isBlocking && timeout != Time::Zero ) {
			mSSLSocket->setReceiveTimeout( Time::Zero );
			mSSLSocket->setSendTimeout( Time::Zero );
		}

		if ( mStatus == Socket::Error ) {
			disconnect();
			mStatus = Socket::Error;
			return mStatus;
		}

		// A TLS 1.2 session is resumable now; a TLS 1.3 one is replaced once a ticket arrives.
		saveSession();
	} else {
		MbedTLSSocket* oldSocket =
			reinterpret_cast<MbedTLSSocket*>( mSSLSocket->mRestoreSession->mImpl );
		// set_session copies the session, so the old socket may replace it afterwards. The
		// handshake runs on the first send or receive.
		Lock l( oldSocket->mSessionMutex );
		if ( NULL != oldSocket->mSSLSession )
			mbedtls_ssl_set_session( &mSSLContext, oldSocket->mSSLSession );
	}

	mConnected = true;
	mStatus = Socket::Done;

	return mStatus;
}

void MbedTLSSocket::disconnect() {
	if ( mInitialized ) {
		mbedtls_ssl_free( &mSSLContext );
		mbedtls_ssl_config_free( &mSSLConfig );
		mbedtls_ctr_drbg_free( &mCtrDrbg );
		mbedtls_entropy_free( &mEntropy );
		mInitialized = false;
	}

	mConnected = false;
	mStatus = Socket::Disconnected;
}

Socket::Status MbedTLSSocket::send( const void* data, std::size_t size ) {
	Socket::Status err;
	size_t sent = 0;
	const char* ptr = (const char*)data;

	while ( size > 0 ) {
		err = send( ptr, size, sent );

		if ( err != Socket::Done ) {
			return err;
		}

		ptr += sent;
		size -= sent;
	}

	return Socket::Done;
}

Socket::Status MbedTLSSocket::send( const void* data, std::size_t size, std::size_t& sent ) {
	sent = 0;

	if ( size == 0 )
		return Socket::Done;

	int ret;
	while ( ( ret = mbedtls_ssl_write( &mSSLContext, (const unsigned char*)data, size ) ) ==
			MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET )
		saveSession();

	if ( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) {
		ret = 0; // non blocking io
	} else if ( ret <= 0 ) {
		disconnect();
		return Socket::Disconnected;
	}

	sent = ret;
	return Socket::Done;
}

Socket::Status MbedTLSSocket::receive( void* data, std::size_t size, std::size_t& received ) {
	received = 0;

	int ret;
	for ( ;; ) {
		ret = mbedtls_ssl_read( &mSSLContext, (unsigned char*)data, size );
		if ( ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET ) {
			saveSession();
			continue;
		}
		// A TLS 1.3 NewSessionTicket is first reported as WANT_READ while Mbed TLS keeps the
		// record buffered; nothing is left on the socket to wait for, so process it now.
		if ( ret == MBEDTLS_ERR_SSL_WANT_READ && mbedtls_ssl_check_pending( &mSSLContext ) )
			continue;
		break;
	}

	if ( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ) {
		return Socket::NotReady;
	} else if ( ret <= 0 ) {
		disconnect();
		return Socket::Disconnected;
	}

	received = ret;
	return Socket::Done;
}

}}} // namespace EE::Network::SSL

#endif
