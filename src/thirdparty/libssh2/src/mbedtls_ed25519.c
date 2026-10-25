/* Ed25519 keys and curve25519-sha256 key exchange for the mbedTLS backend.
 *
 * eepp addition, not part of upstream libssh2. mbedTLS does not implement EdDSA or X25519 key
 * agreement as used by SSH, so the primitives come from Monocypher (src/thirdparty/monocypher),
 * and this file provides the backend functions that crypto.h expects when LIBSSH2_ED25519 is
 * enabled. Security key (sk-ssh-ed25519@openssh.com) private keys are not supported, like the
 * ECDSA security keys of the mbedTLS backend.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "libssh2_priv.h"

#if defined(LIBSSH2_MBEDTLS) && LIBSSH2_ED25519

#include "monocypher.h"
#include "monocypher-ed25519.h"

#include <stdio.h>
#include <string.h>

#define ED25519_METHOD "ssh-ed25519"
#define ED25519_METHOD_LEN (sizeof(ED25519_METHOD) - 1)

static ssh2_ed25519_ctx *
ed25519_ctx_new(void)
{
    return (ssh2_ed25519_ctx *)calloc(1, sizeof(ssh2_ed25519_ctx));
}

void
ssh2_mbedtls_ed25519_free(ssh2_ed25519_ctx *ctx)
{
    if(!ctx)
        return;
    crypto_wipe(ctx, sizeof(*ctx));
    free(ctx);
}

/* Reads the private part of an "openssh-key-v1" ssh-ed25519 entry (the key type string was
 * already consumed): public key, seed + public key, comment and padding. */
static int
ed25519_from_openssh_priv_data(LIBSSH2_SESSION *session,
                               struct string_buf *decrypted,
                               ssh2_ed25519_ctx **out_ctx)
{
    ssh2_ed25519_ctx *ctx;
    unsigned char *pub_key, *priv_key, *comment;
    size_t pub_len = 0, priv_len = 0, comment_len = 0;
    unsigned char seed[SSH2_ED25519_KEY_LEN];
    unsigned char derived_public[SSH2_ED25519_KEY_LEN];
    unsigned char i = 1;

    if(ssh2_get_string(decrypted, &pub_key, &pub_len) ||
       pub_len != SSH2_ED25519_KEY_LEN)
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "Wrong ED25519 public key length");

    if(ssh2_get_string(decrypted, &priv_key, &priv_len) ||
       priv_len != SSH2_ED25519_PRIVATE_KEY_LEN)
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "Wrong ED25519 private key length");

    if(ssh2_get_string(decrypted, &comment, &comment_len))
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "Unable to read ED25519 key comment");

    /* Padding: 1, 2, 3... up to the cipher block size. */
    while(decrypted->dataptr < decrypted->data + decrypted->len) {
        if(*decrypted->dataptr != i)
            return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                                  "Wrong ED25519 key padding");
        i++;
        decrypted->dataptr++;
    }

    ctx = ed25519_ctx_new();
    if(!ctx)
        return ssh2_err(session, LIBSSH2_ERROR_ALLOC,
                              "Unable to allocate ED25519 key");

    /* Rebuild the key pair from the seed and check it against both stored public keys, so
       a corrupted file is reported instead of producing bad signatures. */
    memcpy(seed, priv_key, sizeof(seed));
    crypto_ed25519_key_pair(ctx->secret_key, derived_public, seed);
    if(crypto_verify32(derived_public, pub_key) != 0 ||
       crypto_verify32(derived_public, priv_key + SSH2_ED25519_KEY_LEN) != 0) {
        crypto_wipe(derived_public, sizeof(derived_public));
        ssh2_mbedtls_ed25519_free(ctx);
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "ED25519 private key does not match its "
                              "public key");
    }
    memcpy(ctx->public_key, derived_public, sizeof(ctx->public_key));
    ctx->has_private = 1;
    *out_ctx = ctx;
    return 0;
}

/* Parses decrypted "openssh-key-v1" data holding an ssh-ed25519 key. */
static int
ed25519_from_openssh_decrypted(LIBSSH2_SESSION *session,
                               struct string_buf *decrypted,
                               ssh2_ed25519_ctx **out_ctx)
{
    unsigned char *type = NULL;
    size_t type_len = 0;

    if(ssh2_get_string(decrypted, &type, &type_len) || !type)
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "Public key type in decrypted key data "
                              "not found");

    if(type_len != ED25519_METHOD_LEN ||
       memcmp(type, ED25519_METHOD, ED25519_METHOD_LEN) != 0)
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "Private key is not an ED25519 key");

    return ed25519_from_openssh_priv_data(session, decrypted, out_ctx);
}

int
ssh2_ed25519_new_priv(ssh2_ed25519_ctx **ed_ctx, LIBSSH2_SESSION *session,
                       const char *filename, const char *blob, size_t blob_len,
                       const char *passphrase)
{
    struct string_buf *decrypted = NULL;
    ssh2_ed25519_ctx *ctx = NULL;
    int rc = ssh2_openssh_pem_parse(session, filename, blob, blob_len,
                                   passphrase, &decrypted);
    if(rc)
        return rc;

    rc = ed25519_from_openssh_decrypted(session, decrypted, &ctx);
    ssh2_string_buf_free(session, decrypted);
    if(rc == 0) {
        if(ed_ctx)
            *ed_ctx = ctx;
        else
            ssh2_mbedtls_ed25519_free(ctx);
    }
    return rc;
}

int
ssh2_ed25519_new_public(ssh2_ed25519_ctx **ed_ctx,
                            LIBSSH2_SESSION *session,
                            const unsigned char *raw_pub_key,
                            const size_t key_len)
{
    ssh2_ed25519_ctx *ctx;

    if(!ed_ctx)
        return -1;

    if(key_len != SSH2_ED25519_KEY_LEN)
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "Wrong ED25519 public key length");

    ctx = ed25519_ctx_new();
    if(!ctx)
        return ssh2_err(session, LIBSSH2_ERROR_ALLOC,
                              "Unable to allocate ED25519 key");

    memcpy(ctx->public_key, raw_pub_key, SSH2_ED25519_KEY_LEN);
    *ed_ctx = ctx;
    return 0;
}

int
ssh2_ed25519_sign(ssh2_ed25519_ctx *ctx, LIBSSH2_SESSION *session,
                      uint8_t **out_sig, size_t *out_sig_len,
                      const uint8_t *message, size_t message_len)
{
    unsigned char *sig;

    *out_sig = NULL;
    *out_sig_len = 0;

    if(!ctx || !ctx->has_private)
        return ssh2_err(session, LIBSSH2_ERROR_PROTO,
                              "ED25519 key has no private part");

    sig = SSH2_ALLOC(session, SSH2_ED25519_SIG_LEN);
    if(!sig)
        return ssh2_err(session, LIBSSH2_ERROR_ALLOC,
                              "Unable to allocate ED25519 signature");

    crypto_ed25519_sign(sig, ctx->secret_key, message, message_len);
    *out_sig = sig;
    *out_sig_len = SSH2_ED25519_SIG_LEN;
    return 0;
}

int
ssh2_ed25519_verify(ssh2_ed25519_ctx *ctx, LIBSSH2_SESSION *session,
                      const uint8_t *s,
                        size_t s_len, const uint8_t *m, size_t m_len)
{
    (void)session;
    if(!ctx || s_len != SSH2_ED25519_SIG_LEN)
        return -1;

    return crypto_ed25519_check(s, ctx->public_key, m, m_len) == 0 ? 0 : -1;
}

int
ssh2_curve25519_new(LIBSSH2_SESSION *session, uint8_t **out_public_key,
                        uint8_t **out_private_key)
{
    unsigned char secret[SSH2_ED25519_KEY_LEN];
    unsigned char *priv = NULL, *pub = NULL;

    if(ssh2_random(secret, sizeof(secret)))
        return -1;

    if(out_private_key) {
        priv = SSH2_ALLOC(session, SSH2_ED25519_KEY_LEN);
        if(!priv)
            goto fail;
        memcpy(priv, secret, SSH2_ED25519_KEY_LEN);
    }

    if(out_public_key) {
        pub = SSH2_ALLOC(session, SSH2_ED25519_KEY_LEN);
        if(!pub)
            goto fail;
        /* crypto_x25519 clamps the scalar itself. */
        crypto_x25519_public_key(pub, secret);
    }

    crypto_wipe(secret, sizeof(secret));
    if(out_private_key)
        *out_private_key = priv;
    if(out_public_key)
        *out_public_key = pub;
    return 0;

fail:
    crypto_wipe(secret, sizeof(secret));
    if(priv) {
        crypto_wipe(priv, SSH2_ED25519_KEY_LEN);
        SSH2_FREE(session, priv);
    }
    if(pub)
        SSH2_FREE(session, pub);
    return -1;
}

int
ssh2_curve25519_gen_k(ssh2_bn **k,
                          uint8_t private_key[SSH2_ED25519_KEY_LEN],
                          uint8_t server_public_key[SSH2_ED25519_KEY_LEN])
{
    static const unsigned char zero[SSH2_ED25519_KEY_LEN] = { 0 };
    unsigned char shared[SSH2_ED25519_KEY_LEN];
    int rc = -1;

    if(!k || !*k)
        return -1;

    crypto_x25519(shared, private_key, server_public_key);

    /* A low order server point yields an all-zero secret: reject it like
       OpenSSH does. */
    if(crypto_verify32(shared, zero) != 0)
        rc = ssh2_bn_from_bin(k, shared, SSH2_ED25519_KEY_LEN) == 0
             ? 0 : -1;

    crypto_wipe(shared, sizeof(shared));
    return rc;
}

/* Builds the public key blob of an OpenSSH format ssh-ed25519 private key, for
   ssh2_pub_privkey() when no ".pub" file is given. Returns -1 without
   setting a session error when the data is not an ssh-ed25519 key, so callers
   can keep their own error. */
int
ssh2_mbedtls_pub_priv_openssh_ed25519(LIBSSH2_SESSION *session,
                                          char **method,
                                          unsigned char **pubkeydata,
                                          size_t *pubkeydata_len,
                                          const char *filename,
                                          const char *filedata,
                                          size_t filedata_len,
                                          const char *passphrase)
{
    struct string_buf *decrypted = NULL;
    ssh2_ed25519_ctx *ctx = NULL;
    char *method_buf = NULL;
    unsigned char *key = NULL, *p;
    size_t key_len = 4 + ED25519_METHOD_LEN + 4 + SSH2_ED25519_KEY_LEN;
    int rc;

    if(ssh2_openssh_pem_parse(session, filename, filedata, filedata_len,
                              passphrase, &decrypted))
        return -1;

    rc = ed25519_from_openssh_decrypted(session, decrypted, &ctx);
    ssh2_string_buf_free(session, decrypted);
    if(rc)
        return -1;

    method_buf = SSH2_ALLOC(session, ED25519_METHOD_LEN + 1);
    key = SSH2_ALLOC(session, key_len);
    if(!method_buf || !key) {
        if(method_buf)
            SSH2_FREE(session, method_buf);
        if(key)
            SSH2_FREE(session, key);
        ssh2_mbedtls_ed25519_free(ctx);
        return ssh2_err(session, LIBSSH2_ERROR_ALLOC,
                              "Unable to allocate ED25519 public key");
    }

    memcpy(method_buf, ED25519_METHOD, ED25519_METHOD_LEN + 1);
    p = key;
    ssh2_store_str(&p, ED25519_METHOD, ED25519_METHOD_LEN);
    ssh2_store_u32(&p, SSH2_ED25519_KEY_LEN);
    memcpy(p, ctx->public_key, SSH2_ED25519_KEY_LEN);
    ssh2_mbedtls_ed25519_free(ctx);

    *method = method_buf;
    *pubkeydata = key;
    *pubkeydata_len = key_len;
    return 0;
}

#endif /* LIBSSH2_MBEDTLS && LIBSSH2_ED25519 */
