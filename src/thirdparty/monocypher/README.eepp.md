# Monocypher 4.0.3

Unmodified copy of `src/monocypher.{c,h}` and `src/optional/monocypher-ed25519.{c,h}` from the
Monocypher 4.0.3 release (https://monocypher.org, dual licensed CC0 / BSD-2-Clause, see
`LICENCE.md`).

eepp uses it to give the vendored libssh2 mbedTLS backend Ed25519 keys and the curve25519-sha256
key exchange, which mbedTLS does not implement (see
`src/thirdparty/libssh2/src/mbedtls_ed25519.c`). It is only compiled into `libssh2-static`.
