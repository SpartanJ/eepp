#ifndef LIBSSH2_MBEDTLS_H
#define LIBSSH2_MBEDTLS_H
/* Copyright (C) Art <https://github.com/wildart>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define SSH2_CRYPTO_ENGINE libssh2_mbedtls
#define SSH2_CRYPTO_ENGINE_NAME "mbedTLS"

#include <mbedtls/version.h>

#if MBEDTLS_VERSION_NUMBER < 0x03010000
#  error "mbedTLS 3.1.0 or greater required"
#endif
#if MBEDTLS_VERSION_NUMBER < 0x04000000 && !defined(MBEDTLS_CTR_DRBG_C)
#  error "MBEDTLS_CTR_DRBG_C is required for mbedTLS 3.x."
#endif
#if !defined(MBEDTLS_ECDH_C) || \
    !defined(MBEDTLS_ECDSA_C) || \
    !defined(MBEDTLS_ECP_DP_SECP256R1_ENABLED) || \
    !defined(MBEDTLS_ECP_DP_SECP384R1_ENABLED) || \
    !defined(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
#  error "ECDH/ECDSA with nistp256, nistp384, nistp521 curves is required"
#endif

#include <mbedtls/platform.h>
#include <mbedtls/md.h>
#if MBEDTLS_VERSION_NUMBER < 0x04000000
#include <mbedtls/rsa.h>
#include <mbedtls/bignum.h>
#include <mbedtls/cipher.h>
#ifdef MBEDTLS_GCM_C
# include <mbedtls/gcm.h>
#endif
#ifdef MBEDTLS_ECDH_C
# include <mbedtls/ecdh.h>
#endif
#ifdef MBEDTLS_ECDSA_C
# include <mbedtls/ecdsa.h>
#endif
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#endif
#include <mbedtls/pk.h>
#include <mbedtls/error.h>

/* Define which features are supported. */
#if defined(MBEDTLS_MD5_C)
#define LIBSSH2_MD5 1
#else
#define LIBSSH2_MD5 0
#endif

#if defined(MBEDTLS_RIPEMD160_C)
#define LIBSSH2_HMAC_RIPEMD 1
#else
#define LIBSSH2_HMAC_RIPEMD 0
#endif
#define LIBSSH2_HMAC_SHA256 1
#define LIBSSH2_HMAC_SHA512 1

#define LIBSSH2_AES_CBC 1
#define LIBSSH2_AES_CTR 1
#ifdef MBEDTLS_GCM_C
#define LIBSSH2_AES_GCM 1
#else
#define LIBSSH2_AES_GCM 0
#endif
#define LIBSSH2_BLOWFISH 0
#define LIBSSH2_RC4 0
#define LIBSSH2_CAST 0
#ifdef MBEDTLS_DES_C
#define LIBSSH2_3DES 1
#else
#define LIBSSH2_3DES 0
#endif

#define LIBSSH2_RSA 1
#define LIBSSH2_RSA_SHA1 1
#define LIBSSH2_RSA_SHA2 1
#define LIBSSH2_DSA 0
#define LIBSSH2_ECDSA 1
#define LIBSSH2_ED25519 1 /* eepp: Monocypher, see mbedtls_ed25519.c */
#define LIBSSH2_MLKEM 0

#include "crypto_config.h"

/*******************************************************************/
/*
 * mbedTLS backend: HMAC functions
 */

/* eepp: retain the classic digest API. PSA is disabled in our Mbed TLS build;
   do not enable global PSA key slots without a platform-wide threading setup. */
#define ssh2_hmac_ctx mbedtls_md_context_t
#define ssh2_hash_ctx mbedtls_md_context_t
#define ssh2_hash_alg mbedtls_md_type_t
#define ssh2_hash_update(ctx, d, l) \
    (mbedtls_md_update(ctx, (const unsigned char *)(d), l) == 0)
#define ssh2_hmac_update(ctx, d, l) \
    (mbedtls_md_hmac_update(ctx, (const unsigned char *)(d), l) == 0)
#define SSH2_SHA1_ALG MBEDTLS_MD_SHA1
#define SSH2_SHA256_ALG MBEDTLS_MD_SHA256
#define SSH2_SHA384_ALG MBEDTLS_MD_SHA384
#define SSH2_SHA512_ALG MBEDTLS_MD_SHA512
#if LIBSSH2_MD5 || LIBSSH2_MD5_PEM
#define SSH2_MD5_ALG MBEDTLS_MD_MD5
#endif
#if LIBSSH2_HMAC_RIPEMD
#define SSH2_RIPEMD160_HMAC MBEDTLS_MD_RIPEMD160
#endif

/*******************************************************************/
/*
 * mbedTLS backend: RSA functions
 */

#define ssh2_rsa_ctx           mbedtls_rsa_context

/*******************************************************************/
/*
 * mbedTLS backend: ECDSA structures
 */

#if LIBSSH2_ECDSA
typedef enum {
    SSH2_EC_CURVE_NISTP256 = MBEDTLS_ECP_DP_SECP256R1,
    SSH2_EC_CURVE_NISTP384 = MBEDTLS_ECP_DP_SECP384R1,
    SSH2_EC_CURVE_NISTP521 = MBEDTLS_ECP_DP_SECP521R1
} ssh2_curve_type;

#define ssh2_ecdsa_ctx         mbedtls_ecdsa_context
#define ssh2_ec_key            mbedtls_ecp_keypair
#endif /* LIBSSH2_ECDSA */

/*******************************************************************/
/*
 * mbedTLS backend: Cipher Context structure
 */

/* Unified cipher context structure.
   Handles both regular ciphers and AEAD ciphers (GCM)
   using a union to optimize memory usage. */
struct ssh2_mbed_cipher_ctx {
    int is_aesgcm;
    union {
        mbedtls_cipher_context_t cipher;
#if LIBSSH2_AES_GCM
        struct {
            mbedtls_gcm_context ctx;
            unsigned char iv[12];
        } gcm;
#endif
    } u;
};

#define ssh2_cipher_ctx        struct ssh2_mbed_cipher_ctx

#define SSH2_CIPHER_T(algo)    mbedtls_cipher_type_t algo

#define ssh2_cipher_aes256ctr  MBEDTLS_CIPHER_AES_256_CTR
#define ssh2_cipher_aes192ctr  MBEDTLS_CIPHER_AES_192_CTR
#define ssh2_cipher_aes128ctr  MBEDTLS_CIPHER_AES_128_CTR
#define ssh2_cipher_aes256     MBEDTLS_CIPHER_AES_256_CBC
#define ssh2_cipher_aes192     MBEDTLS_CIPHER_AES_192_CBC
#define ssh2_cipher_aes128     MBEDTLS_CIPHER_AES_128_CBC
#if LIBSSH2_3DES
#define ssh2_cipher_3des       MBEDTLS_CIPHER_DES_EDE3_CBC
#endif
#define ssh2_cipher_aes128gcm  MBEDTLS_CIPHER_AES_128_GCM
#define ssh2_cipher_aes256gcm  MBEDTLS_CIPHER_AES_256_GCM
#define ssh2_cipher_chacha20   MBEDTLS_CIPHER_CHACHA20_POLY1305

/*******************************************************************/
/*
 * mbedTLS backend: BigNumber Support
 */

#define ssh2_bn                        mbedtls_mpi
#define ssh2_bn_set_word(bn, word)     mbedtls_mpi_lset(bn, (long long)(word))
#define ssh2_bn_to_bin(bn, bin) \
    mbedtls_mpi_write_binary(bn, bin, mbedtls_mpi_size(bn))
#define ssh2_bn_bytes(bn)              mbedtls_mpi_size(bn)
#define ssh2_bn_bits(bn)               mbedtls_mpi_bitlen(bn)

/*******************************************************************/
/*
 * mbedTLS backend: Diffie-Hellman support.
 */

#define ssh2_dh_ctx            mbedtls_mpi *

/* Default generate and safe prime sizes for
   diffie-hellman-group-exchange-sha1 */
#define SSH2_DH_GEX_MINGROUP     2048
#define SSH2_DH_GEX_OPTGROUP     4096
#define SSH2_DH_GEX_MAXGROUP     8192

#define SSH2_DH_MAX_MODULUS_BITS 16384

/* eepp: mbedTLS has no SSH Ed25519/X25519 primitives; use Monocypher. */
#if LIBSSH2_ED25519
typedef struct {
    unsigned char public_key[32];
    /* Monocypher layout: 32-byte seed followed by the public key. */
    unsigned char secret_key[64];
    int has_private;
} ssh2_mbedtls_ed25519_ctx;
#define ssh2_ed25519_ctx ssh2_mbedtls_ed25519_ctx
#define ssh2_ed25519_free(ctx, session) ssh2_mbedtls_ed25519_free(ctx)

void ssh2_mbedtls_ed25519_free(ssh2_ed25519_ctx *ctx);
int ssh2_mbedtls_pub_priv_openssh_ed25519(LIBSSH2_SESSION *session,
                                         char **method,
                                         unsigned char **pubkeydata,
                                         size_t *pubkeydata_len,
                                         const char *filename,
                                         const char *filedata,
                                         size_t filedata_len,
                                         const char *passphrase);
#endif

#endif /* LIBSSH2_MBEDTLS_H */
