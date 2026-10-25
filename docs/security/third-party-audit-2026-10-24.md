# Third-party security audit — 2026-10-24

Repository baseline: `dfc1a1941` (Brotli 1.2.1 upgrade included).
Scope: the 45 source trees in `src/thirdparty`, public third-party headers, nested
components, and the build configuration that selects them. This is a source inventory
and advisory/reachability review, not a deployed-binary SBOM or a claim that all code
is vulnerability-free. No dependency sources were changed during this audit.

The companion [machine-readable inventory](third-party-inventory-2026-10-24.json)
records version evidence, repository revisions, source fingerprints, upstream URLs,
and the successful NVD queries. `version: null` means the upstream release has not
been established; a repository commit or copyright year is not a substitute for it.

## Recommended cleanup order

| Order | Component | Current source | Action | Reason and qualification |
| --- | --- | --- | --- | --- |
| 1 | libssh2 | 1.11.1 plus local crypto integration | Backport verified fixes or pin a reviewed upstream revision | Relevant transport and SFTP memory-corruption fixes are absent. Official latest release is still 1.11.1, so a version-only bump cannot resolve this. |
| 2 | libpng | 1.6.37 | Upgrade to 1.6.59 | CVE-2026-33636 affects the compiled ARM Neon palette-expansion path. FreeType can reach it through PNG color glyphs. |
| 3 | NanoSVG | Modified FLTK-derived snapshot | Backport reviewed numeric-range fixes with regression cases | CVE-2026-88366 and CVE-2026-88368 sites remain. The cited upstream issues are open; an arbitrary latest-master update is not a verified fix. |
| 4 | libzip | 0.10 plus local hardening | Resolve residual arithmetic checks; plan a modern libzip migration | Local changes address one old CVE, but other unchecked arithmetic remains. Bounded VFS opens and the public unrestricted ZIP API have different exposure. |
| 5 | Mbed TLS | 3.6.0 | Upgrade within LTS to 3.6.7 | Substantial security backlog. Many high-scoring findings require unused APIs, a TLS server role, unusual compiler behavior, or deployment conditions. Do not characterize the ordinary eepp HTTPS client as a confirmed RCE. |
| 6 | libvorbis | 1.3.6 | Upgrade to 1.3.7 | The CVE-2018-10393 encoder bounds fix is absent. eepp includes an encoder, so this is not just a decoder-only dependency. |
| 7 | Bundled SDL2 source | 2.0.12 | Reconcile Android source with the newer SDL2 pin | Old source has high-severity surface/blit issues. Windows/iOS download pins and system SDL are separate versions; a default BMP exploit path in eepp was not established. |
| 8 | simdutf | 9.2.0 | Update to at least 9.2.1 | High upstream advisory affects Base64 APIs that eepp does not currently call. Useful small maintenance update, not a confirmed current attack path. |
| 9 | Monocypher | 4.0.2 | Update to 4.0.3, preferably with libssh2 work | Upstream fixes compiler-dependent EdDSA/Ed25519 timing leakage. Signing is used by our SSH backend; applicability depends on generated code. No high CVE for that issue was established. |

Orders indicate engineering priority for this repository, not a new CVSS score.
Targets are the upstream releases verified during this audit, rather than promises
that future releases will have the same security status.

## Findings with concrete source evidence

### libssh2: highest priority

Version: [`include/libssh2.h`](../../src/thirdparty/libssh2/include/libssh2.h).
The SFTP module calls `libssh2_sftp_init()` and `libssh2_sftp_open_ex()` in
[`sshconnection.cpp`](../../src/modules/vfs/src/eepp/vfs/sftp/sshconnection.cpp)
and [`sftpprovider.cpp`](../../src/modules/vfs/src/eepp/vfs/sftp/sftpprovider.cpp).

| Advisory | Severity reported | Local evidence | Disposition |
| --- | --- | --- | --- |
| [CVE-2026-55200](https://nvd.nist.gov/vuln/detail/CVE-2026-55200) | Critical, CNA CVSS 4.0 9.2 | The full-packet branch in `src/transport.c` checks `packet_length < 1` without the upper-bound check added upstream. `crypt.c` registers ChaCha20-Poly1305 with `LIBSSH2_CRYPT_FLAG_REQUIRES_FULL_PACKET`. | Relevant to supported SSH use, including the Mbed TLS backend; AES-GCM being disabled there does not remove the ChaCha20 path. |
| [CVE-2026-66035](https://nvd.nist.gov/vuln/detail/CVE-2026-66035) | High, CVSS 3.1 7.5 | ETM full-packet processing allocates `decrypt_size` then copies `blocksize - 1` bytes without the added minimum-size guard. ETM MACs are registered in `src/mac.c`. | Relevant transport memory-corruption risk from a malicious SSH peer. |
| [CVE-2026-66032](https://nvd.nist.gov/vuln/detail/CVE-2026-66032) | High, CVSS 3.1 8.8 | `sftp_open()` frees `data` after an FX_OK response without the upstream `data = NULL` before waiting for a handle. | Relevant SFTP error-path double-free risk. |
| [CVE-2026-66033](https://nvd.nist.gov/vuln/detail/CVE-2026-66033) | High, CVSS 3.1 7.5 | OpenSSL cipher bounds checks are not backported. | Relevant when built with `--with-openssl`; Mbed TLS backend has `LIBSSH2_AES_GCM = 0`. |
| [CVE-2026-66034](https://nvd.nist.gov/vuln/detail/CVE-2026-66034), [CVE-2026-58050](https://nvd.nist.gov/vuln/detail/CVE-2026-58050) | High, respectively 7.5 and 7.0 | Vulnerable publickey subsystem implementation is included. | No eepp call to the publickey-list subsystem was found. Lower priority than transport/SFTP fixes, but include in a complete refresh. |
| [CVE-2026-7598](https://nvd.nist.gov/vuln/detail/CVE-2026-7598) | Scores vary by provider | Username/password length arithmetic fixes are not present. | API-input preconditions matter; not equivalent to a malicious packet reaching ordinary small credentials. Include in refresh. |

Verified upstream fixes:

- [97acf3df](https://github.com/libssh2/libssh2/commit/97acf3dfda80c91c3a8c9f2372546301d4a1a7a8): packet upper bound.
- [42e33d81](https://github.com/libssh2/libssh2/commit/42e33d81577ed4b95d4b4f6f845e5ee8efe5eeb4): ETM minimum size.
- [5e477614](https://github.com/libssh2/libssh2/commit/5e4776146552d898b9c0e1b313cd093fa8dc92d0): SFTP dangling pointer.
- [a2ed82d4](https://github.com/libssh2/libssh2/commit/a2ed82d40964bbc0d64cd717aa0a5a892117d2e6): OpenSSL cipher bounds.
- [a13bb6c7](https://github.com/libssh2/libssh2/commit/a13bb6c773f0d55ad1628cede57e99803cd898d9): publickey response bounds.
- [256d04b6](https://github.com/libssh2/libssh2/commit/256d04b60d80bf1190e96b0ad1e91b2174d744b1): authentication length arithmetic.

The [official release metadata](https://github.com/libssh2/libssh2/releases/latest)
still selects 1.11.1. Backports need adaptation because upstream renamed many
`_libssh2_*` internals. Preserve our Mbed TLS compatibility and Monocypher-based
Ed25519/X25519 integration. Include the additional medium transport, SFTP and
publickey advisories in the same review; do not assume the six patches above are a
complete replacement for reviewing the upstream changes since 1.11.1.

Host-key validation remains necessary, but it should not be treated as a replacement
for fixing transport processing, which runs during connection establishment.
No exploit was run against eepp in this audit.

### libpng: ARM/AArch64 decoding path

[`png.h`](../../src/thirdparty/libpng/png.h) identifies 1.6.37.
The vulnerable loops are present in
[`arm/palette_neon_intrinsics.c`](../../src/thirdparty/libpng/arm/palette_neon_intrinsics.c).
`nm -g libs/macosx/thirdparty/liblibpng.a` confirms definitions of both palette
expansion Neon functions in the existing local archive.

[CVE-2026-33636 / GHSA-wjr5-c57x-95m2](https://github.com/pnggroup/libpng/security/advisories/GHSA-wjr5-c57x-95m2)
affects 1.6.36–1.6.55, with ARM Neon palette expansion enabled. The final partial
chunk can read and write before the row buffer. It is fixed in 1.6.56. Our
FreeType [`pngshim.c`](../../src/thirdparty/freetype2/src/sfnt/pngshim.c) calls
`png_set_palette_to_rgb()`, optionally `png_set_tRNS_to_alpha()`, and `png_read_image()`.
Consequently a PNG color glyph in a font is a supported route to the affected code.
This is not an assertion that the ordinary SOIL2 PNG loader uses libpng: it uses stb_image.

Upgrade to the [current 1.6.59 release](https://www.libpng.org/pub/png/libpng.html),
which also includes subsequent fixes. Preserve build/configuration changes and
validate ARM color-font decoding as well as ordinary font loading.

Other high-scoring matches need distinct treatment:

| Advisory | Repository assessment |
| --- | --- |
| [CVE-2026-33416](https://github.com/pnggroup/libpng/security/advisories/GHSA-m4pc-p4q3-4c7j) | Pointer aliasing bug is present. The required free/re-set ordering before continued transforms was not established in the FreeType wrapper. Covered by the proposed upgrade. |
| [CVE-2026-25646](https://github.com/pnggroup/libpng/security/advisories/GHSA-g8hp-mq4h-rqm3) | No consumer call to palette quantization was found. |
| CVE-2025-64720, CVE-2025-65018, CVE-2025-66293 | Affect simplified image APIs; eepp's FreeType wrapper uses the conventional API. No simplified API consumers outside vendored code were found. |
| CVE-2026-22801 | Simplified PNG write API issue, rated moderate upstream despite a higher NVD score. No relevant consumer found. |
| CVE-2026-22695 | Regression introduced in 1.6.51; not applicable to 1.6.37. |

### NanoSVG: parsing and rasterization

The snapshot has FLTK changes and local color/style behavior. There is no reliable
embedded release identifier. It is used by
[`Image`](../../src/eepp/graphics/image.cpp) and the SVG glyph hooks in
[`FontTrueType`](../../src/eepp/graphics/fonttruetype.cpp).

| Advisory | Current code | Assessment |
| --- | --- | --- |
| [CVE-2026-88366, upstream #294](https://github.com/memononen/nanosvg/issues/294) | `nsvg__pathArcTo()` converts a possibly non-finite delta angle to `int`, without a finite/range check. | Reported CVSS 3.1 7.5; vulnerable operation and parser caller present. |
| [CVE-2026-88368, upstream #292](https://github.com/memononen/nanosvg/issues/292) | `nsvg__addActive()` converts scaled geometry to `int` without checking representability. | Reported CVSS 3.1 7.5; vulnerable operation and rasterizer caller present. |
| [CVE-2026-88367, upstream #293](https://github.com/memononen/nanosvg/issues/293) | `nsvg__curveDivs()` divides by an angle that can round to zero before conversion to `int`. | Medium-scored related issue; include in the same hardening change. |

These are numeric undefined-behavior/availability findings. The cited reports
include sanitizer-triggered termination; this audit does not establish a release
build RCE or deterministic release-build crash. Fixes need source comparison and
regression tests rather than inventing a safe release number. The upstream issues
were open when checked.

### libzip: old release with meaningful local changes

[`zipconf.h`](../../src/thirdparty/libzip/zipconf.h) says 0.10, but the source is not
an unmodified 0.10 release. In particular:

- CVE-2012-1162's zero-entry allocation/write condition is guarded in
  [`zip_open.c`](../../src/thirdparty/libzip/zip_open.c), and the empty-directory
  case returns before the original `do` loop. Do not list this as unpatched.
- CVE-2017-12858's upstream fix removes a second free of a `zip_buffer` in
  `_zip_dirent_process_winzip_aes()` failure handling. That machinery is absent
  here. A scanner's version-only match is insufficient. ZIP64 advisory
  CVE-2017-14107 likewise targets code not present in this old implementation.
- The allocation bounds check associated with CVE-2015-2331 is absent from
  `_zip_cdir_new()`, and `_zip_cdir_grow()` also multiplies without an allocation
  overflow check. Exact reachability depends on entry-count handling and platform
  size; the initial count here is only 16-bit, so this is not a reproduced CVE.
- The expression `cd->offset + cd->size` still adds two `unsigned int` fields
  before checking the end of the directory. This is the family of offset/size
  arithmetic covered by CVE-2012-1163 (legacy CVSS 2.0 6.8). It warrants a focused
  review even though it falls below this audit's nominal high threshold.

The VFS path uses `OpenMode::ReadOnlyBounded` / `ZIP_LIMIT_METADATA`, which constrains
large directories. The ordinary public `Zip::open()` uses `zip_open(..., 0, ...)`.
Do not apply the VFS mitigation to every caller.

Sources: [libzip security history](https://libzip.org/security/),
[CVE-2015-2331 allocation fix](https://github.com/php/php-src/commit/ef8fc4b53d92fbfcd8ef1abbd6f2f5fe2c4a11e5),
[CVE-2017-12858 actual fix](https://github.com/nih-at/libzip/commit/2217022b7d1142738656d891e00b3d2d9179b796).
The verified current upstream release is 1.12.1. Migration needs work on our
custom APIs, file-descriptor ownership, metadata limits and build integration;
it is not a drop-in replacement for the current fork.

### Mbed TLS: upgrade, with precise exposure accounting

Default TLS backend is selected in `premake5.lua`; OpenSSL is optional.
[`build_info.h`](../../src/thirdparty/mbedtls/include/mbedtls/build_info.h)
identifies 3.6.0. The [3.6.7 LTS release](https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7)
is the verified update target, avoiding a simultaneous 4.x migration.

The ordinary [`MbedTLSSocket`](../../src/eepp/network/ssl/backend/mbedtls/mbedtlssocket.cpp)
is a stream TLS client. It sets the hostname, uses certificate verification when
requested, and disables session tickets. This excludes several high findings from
its usual configuration. The following is a triage of high upstream or high NVD
matches, not a claim they all affect that client:

| Advisory | Assessment of current callers/configuration |
| --- | --- |
| CVE-2024-45158 | Unsafe ECDSA conversion `bits` argument. No direct eepp/libssh2 calls to these conversion APIs found; upstream states internal calls use safe values. |
| CVE-2024-45159 | TLS 1.3 server with optional client authentication; eepp socket is a client. |
| CVE-2024-49195 | Opaque-key serialization path; no relevant consumer found. |
| CVE-2025-47917, CVE-2025-48965, CVE-2026-34874 | Distinguished-name/ASN.1 construction API conditions, with no relevant application caller found. |
| CVE-2025-52496 | AESNI capability-cache race is present, with `MBEDTLS_AESNI_C` enabled. NVD rates it high (7.8), upstream medium. Applies to x86 multithreaded code only with problematic compiler ordering and a powerful local side-channel attacker. Upstream has not observed it with Clang or modern GCC. Current macOS ARM build is outside that architecture. |
| CVE-2026-34875 | PSA FFDH export size issue. No consumer use of the affected PSA export path found in eepp/libssh2. |
| CVE-2026-34872 | FFDH contributory-behavior issue; upstream explicitly says TLS does not require that property. SSH uses libssh2's separate DH implementation. |
| CVE-2026-34873 | TLS 1.3 server session resumption; ordinary client role/ticket configuration excludes it. |
| CVE-2026-34876 | Oversized tag argument to multipart CCM finish; no direct consumer use found. |
| CVE-2026-25835 | Cloned PSA RNG state; no supported path that clones a process and then continues using its copied PSA RNG was established. |
| CVE-2026-25833 | Fallback IPv6 parser when platform lacks `AF_INET6`/`inet_pton`; usual desktop platform path differs. Upstream rates it low despite higher database scoring. |
| CVE-2026-49300 | Malformed certificate interpreted as a CA. Upstream's exploit scenario requires accepting untrusted certificates through a different validator/trust-registration process; such a workflow was not found here. |
| CVE-2026-50580 | TLS 1.2 ECDHE-PSK client with oversized configured PSK identity. No PSK configuration calls found. |
| CVE-2026-50579 | Reused PKCS7 context; no PKCS7 caller found. |
| CVE-2026-50713 | Introduced in 3.6.3; 3.6.0 is outside the affected interval. DTLS also differs from the eepp stream TLS path. |

Advisory definitions and fixes: [official security index](https://mbed-tls.readthedocs.io/en/latest/security-advisories/).
The individual documents were read from the official `Mbed-TLS/mbedtls-docs`
source archive, including issues whose NVD descriptions omit configuration details.
The [AESNI advisory](https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-1/)
is particularly useful when assessing x86 distribution builds.

Several additional low/medium fixes apply to this old TLS baseline. Keeping 3.6.0
because an individual high-CVE precondition is absent would leave that backlog.
An upgrade is recommended, but the distinctions above should remain in the record.

### libvorbis: encoder update

[`lib/info.c`](../../src/thirdparty/libvorbis/lib/info.c) identifies 1.3.6.
[`lib/psy.c`](../../src/thirdparty/libvorbis/lib/psy.c) still lacks the loop/index
bounds added for CVE-2018-10393 (CVSS 3.1 7.5). This was verified by comparing the
local file with the official 1.3.7 archive.
[`SoundFileWriterOgg`](../../src/eepp/audio/soundfilewriterogg.cpp) configures the
Vorbis encoder with the supplied sample rate/channel count and invokes its analysis
pipeline. The relevant issue is encoding unusual low sample rates, not ordinary
playback of every Ogg file. Upgrade to 1.3.7 and test those input conditions.

CVE-2018-10392 is another version match involving channel validation; its precise
caller conditions were not reproduced. Include the upstream channel-validation
changes in the refresh.
Source: [upstream 1.3.7 changelog](https://github.com/xiph/vorbis/blob/v1.3.7/CHANGES).

## High findings whose current eepp API conditions were not found

### PCRE2

10.44 is older than the verified 10.49 security release, but these are not all
reachable through our wrapper:

| Advisory | Why ordinary eepp Regex use differs |
| --- | --- |
| CVE-2026-103111 / GHSA-r9hj-j2rw-4q3m | Requires explicitly created/assigned growable JIT stack; no consumer `pcre2_jit_stack_create` or `pcre2_jit_stack_assign` calls found. |
| CVE-2026-86145 / GHSA-3r4p-g7gg-ppmf | Requires DFA matching; no consumer `pcre2_dfa_match` calls found. |
| CVE-2026-89161 | Requires direct JIT match with previously copied subject state; wrapper calls `pcre2_match`, creates fresh match data, and does not request `PCRE2_COPY_MATCHED_SUBJECT`. |
| CVE-2026-89157 | Requires large pattern conversion on 32-bit systems; no consumer pattern-convert calls found. |
| CVE-2025-58050 | Only 10.45; our 10.44 is outside the affected interval. |

Recommend 10.49 as maintenance, not as evidence of a confirmed current wrapper RCE.
Sources: [10.49 advisory](https://github.com/PCRE2Project/pcre2/security/advisories/GHSA-r9hj-j2rw-4q3m),
[DFA advisory](https://github.com/PCRE2Project/pcre2/security/advisories/GHSA-3r4p-g7gg-ppmf),
[JIT state fix](https://github.com/PCRE2Project/pcre2/pull/937),
[supported backports](https://github.com/PCRE2Project/pcre2/blob/main/SUPPORT-LIFECYCLE.md).

### HarfBuzz

8.5.0 falls in the stated ranges for CVE-2024-56732 (Cairo integration) and
CVE-2026-113679 (experimental WASM shaper). Their implementation files are guarded
by `HAVE_CAIRO` and `HAVE_WASM`, neither enabled in our configuration. The eepp
renderer does not call `hb_cairo_glyphs_from_buffer`. These are not confirmed
high-CVE reasons to urgently replace the shaping library in this build.
Medium advisories still justify a normal maintenance review.
Sources: [Cairo advisory](https://github.com/harfbuzz/harfbuzz/security/advisories/GHSA-qmp9-xqm5-jh6m),
[HarfBuzz security](https://github.com/harfbuzz/harfbuzz/security).

### simdutf and Monocypher

simdutf 9.2.0 is within the high
[GHSA-pw55-7mm7-cvhh](https://github.com/simdutf/simdutf/security/advisories/GHSA-pw55-7mm7-cvhh)
range. It affects Base64 decoding into exactly sized output buffers, and is fixed
in 9.2.1. eepp uses Unicode conversion/counting functions; no affected Base64
consumer was found. The related LASX Latin-1 issue is also fixed in 9.2.1, but those
conversion functions were not found among current callers. Neither advisory had a
CVE assigned when checked.

Monocypher 4.0.3 fixes an EdDSA/Ed25519 timing leak in 4.0.2 and earlier with certain
compiler/options combinations. Our SSH integration does sign using Ed25519, so
update it during the SSH refresh. No high-scored CVE was found for that particular
issue. Sources: [known bugs](https://monocypher.org/bugs),
[4.0.3 release](https://github.com/LoupVaillant/Monocypher/releases/tag/4.0.3).

### SDL version selection

| Workflow/source | Version evidence | Assessment |
| --- | --- | --- |
| Tracked `src/thirdparty/SDL2` | `include/SDL_version.h`: 2.0.12 | Android `projects/android-project/app/jni/eepp.mk` sets `SDL_PATH` to this tree. |
| SDL2 download pin | `premake5.lua` and `premake4.lua`: 2.32.10 | Separate from tracked 2.0.12 tree. iOS helper follows this pin. |
| SDL3 download pin | Premake scripts: 3.4.10 | Separate backend; do not apply SDL2 version ranges blindly. |
| System SDL on desktop | Package/toolchain selected at build time | Repository source does not prove installed or redistributed binary version. |

High source matches for the old Android tree include CVE-2020-14409 (blit arithmetic),
CVE-2021-33657 (surface/pixel allocation) and CVE-2022-4743 (GLES renderer allocation
leak). eepp decodes images through SOIL2 and uses its own renderer, so crafted BMP
loading and SDL GLES texture creation were not established as current routes.
Updating the actual Android source is still preferable to carrying a 2.0.12 fork;
changing only a Premake download version does not update it.
Source: [SDL surface-allocation fix](https://github.com/libsdl-org/SDL/commit/8c91cf7dba5193f5ce12d06db1336515851c9ee9).

## Important exclusions and already-fixed issues

| Match | Reason it is not an outstanding high-CVE upgrade finding here |
| --- | --- |
| Brotli CVE-2026-111497 | Already upgraded to 1.2.1 in baseline commit `dfc1a1941`. |
| tinyexpr CVE-2026-88355 | `new_expr()` already allocates `sizeof(te_expr) + extra`, never less than `sizeof(te_expr)`. Local code predates and independently prevents the reported condition. [Original issue](https://github.com/codeplea/tinyexpr/issues/145). |
| FreeType CVE-2025-27363 / CVE-2026-23865 | Our 2.14.1 is newer than the affected versions. Pillow's CVE-2026-112893 is a caller bug in a different project. FreeType's dependent codecs remain separately covered above. |
| libwebp CVE-2023-4863 | Our 1.5.0 includes the fix shipped in 1.3.2. Later matches about Perl Image::WebP's bundled old decoder or an npm wrapper are different products. |
| stb_image's 2023 high advisories | SOIL2 bundles 2.30, beyond those affected releases. CVE-2026-5185 is a medium GIF finding and needs a separate follow-up; do not label stb_image vulnerability-free. |
| dr_libs WAV CVEs | `dr_wav.h` is not vendored or used. We have dr_flac and dr_mp3. Their metadata and whole-file APIs must be assessed independently; WAV advisories are not transferable. |
| zlib CVE-2026-22184 | `contrib/untgz` demonstration utility is absent, not part of the built library. |
| zlib CVE-2026-27820 | Ruby `zlib` binding, not native zlib, despite a misleading broad version/CPE match. |
| zlib CVE-2026-85091 | Concerns non-blocking `gzwrite`/`gzprintf` handling. Our 1.3.1 predates the described regression and no consumer use was found. Older-version scope was still being discussed; if upgrading to 1.3.2, include the subsequent upstream fix rather than blindly replacing 1.3.1. |
| zlib CVE-2026-27171 | CRC combine negative-length issue is below high severity; no affected application API use found. |
| libyaml 2024 emitter/document reports | Current application use is initialized parser/event processing. Several reports concern API misuse or were rejected; do not infer a high parser exposure from the 0.2.5 number. [Maintainer analysis](https://github.com/yaml/libyaml/issues/303). |
| jpeg-compressor CVE-2018-13030 / CVE-2018-13037 | Target `stb_image.c`, which is absent from this import; current image decoder is separately inventoried as stb_image 2.30. |
| Gumbo CVE-2025-6490 | Withdrawn report about Nokogiri fork's `hashmap.c`; our Google-derived tree has no such file. |
| Chipmunk CMS CVEs | Unrelated PHP applications, not Chipmunk2D physics. |
| nlohmann CVE-2024-34363 | Envoy's handling of exceptions, not a nlohmann library version vulnerability. |

## Complete source-tree inventory

`Screened` means no actionable matching high/critical issue was established in this
pass. It does not mean no bugs or no lower-severity advisories exist.
`Snapshot` means upstream release provenance is incomplete; retain the fingerprint
and establish a verified tag/commit before calling a future update routine.

| Component | Version established | Disposition | Local evidence |
| --- | --- | --- | --- |
| args | Unversioned/patched snapshot | Snapshot | [args.hxx](../../src/thirdparty/args/args.hxx) |
| backward-cpp | Unversioned/patched snapshot | Snapshot | [backward.hpp](../../src/thirdparty/backward-cpp/backward.hpp) |
| brotli | 1.2.1 | Fixed | [version.h](../../src/thirdparty/brotli/common/version.h) |
| chipmunk | 6.1.4 | Screened | [chipmunk.h](../../src/modules/physics/include/eepp/thirdparty/chipmunk/chipmunk.h) |
| cpp-unicodelib | Unversioned/patched snapshot | Snapshot | [unicodelib.h](../../src/thirdparty/cpp-unicodelib/unicodelib.h) |
| dr_libs | dr_flac 0.13.2; dr_mp3 0.7.2 | Screened | [dr_flac.h](../../src/thirdparty/dr_libs/dr_flac.h) |
| dtl | 1.21 | Screened | [variables.hpp](../../src/thirdparty/dtl/variables.hpp) |
| efsw | 1.7.2 + 5 commits | Screened | [CMakeLists.txt](../../src/thirdparty/efsw/CMakeLists.txt) |
| emscripten-browser-clipboard | Unversioned/patched snapshot | Snapshot | [emscripten_browser_clipboard.h](../../src/thirdparty/emscripten-browser-clipboard/emscripten_browser_clipboard.h) |
| fast_float | 6.1.6 | Screened | [fast_float.h](../../src/thirdparty/fast_float/include/fast_float/fast_float.h) |
| freetype2 | 2.14.1 | Screened | [freetype.h](../../src/thirdparty/freetype2/include/freetype/freetype.h) |
| fts_fuzzy_match | Unversioned/patched snapshot | Snapshot | [fts_fuzzy_match.h](../../src/thirdparty/fts_fuzzy_match/fts_fuzzy_match.h) |
| glew | 2.1.0 | Screened | [glew.h](../../src/thirdparty/glew/glew.h) |
| glm | 0.9.4.0 | Screened | [setup.hpp](../../src/thirdparty/glm/core/setup.hpp) |
| gumbo-parser | Unversioned/patched snapshot | Snapshot | [gumbo.h](../../src/thirdparty/gumbo-parser/gumbo.h) |
| harfbuzz | 8.5.0 | Conditional | [hb-version.h](../../src/thirdparty/harfbuzz/hb-version.h) |
| imageresampler | 2.2 | Screened | [resampler.h](../../src/thirdparty/imageresampler/resampler.h) |
| jpeg-compressor | jpge 1.05; jpgd snapshot | Snapshot | [jpge.cpp](../../src/thirdparty/jpeg-compressor/jpge.cpp) |
| libogg | 1.3.3 | Screened | [framing.c](../../src/thirdparty/libogg/src/framing.c) |
| libpng | 1.6.37 | Upgrade | [png.h](../../src/thirdparty/libpng/png.h) |
| libssh2 | 1.11.1 + local crypto integration | Patch | [libssh2.h](../../src/thirdparty/libssh2/include/libssh2.h) |
| libvorbis | 1.3.6 | Upgrade | [info.c](../../src/thirdparty/libvorbis/lib/info.c) |
| libwebp | 1.5.0 | Screened | [vp8i_dec.h](../../src/thirdparty/libwebp/src/dec/vp8i_dec.h) |
| libyaml | 0.2.5 | Screened | [config.h](../../src/thirdparty/libyaml/src/config.h) |
| libzip | 0.10 + local hardening | Review | [zipconf.h](../../src/thirdparty/libzip/zipconf.h) |
| mbedtls | 3.6.0 | Upgrade | [build_info.h](../../src/thirdparty/mbedtls/include/mbedtls/build_info.h) |
| md4c | Unversioned/patched snapshot | Snapshot | [md4c.c](../../src/thirdparty/md4c/md4c.c) |
| mojoAL | 2.0.1 | Screened | [mojoal.c](../../src/thirdparty/mojoAL/mojoal.c) |
| mojoAL-SDL3 | 2.9.9 | Screened | [mojoal.c](../../src/thirdparty/mojoAL-SDL3/mojoal.c) |
| monocypher | 4.0.2 | Maintenance | [monocypher.h](../../src/thirdparty/monocypher/monocypher.h) |
| nanosvg | Unversioned/patched snapshot | Patch | [nanosvg.h](../../src/thirdparty/nanosvg/nanosvg.h) |
| nlohmann | 3.12.0 | Screened | [json.hpp](../../src/thirdparty/nlohmann/json.hpp) |
| oniguruma | 6.9.10 | Screened | [oniguruma.h](../../src/thirdparty/oniguruma/oniguruma.h) |
| pcre2 | 10.44 | Conditional | [pcre2.h](../../src/thirdparty/pcre2/src/pcre2.h) |
| portable-file-dialogs | Unversioned/patched snapshot | Snapshot | [portable-file-dialogs.h](../../src/thirdparty/portable-file-dialogs/portable-file-dialogs.h) |
| pugixml | 1.12 | Screened | [pugixml.hpp](../../src/thirdparty/pugixml/pugixml.hpp) |
| SDL2 | 2.0.12 | Conditional | [SDL_version.h](../../src/thirdparty/SDL2/include/SDL_version.h) |
| SheenBidi | 2.9.0 | Screened | [SBVersion.h](../../src/thirdparty/SheenBidi/Headers/SheenBidi/SBVersion.h) |
| simdutf | 9.2.0 | Maintenance | [simdutf.h](../../src/thirdparty/simdutf/simdutf.h) |
| SOIL2 | 1.3.0 snapshot | Screened | [SOIL2.h](../../src/thirdparty/SOIL2/src/SOIL2/SOIL2.h) |
| subprocess | Unversioned/patched snapshot | Snapshot | [subprocess.h](../../src/thirdparty/subprocess/subprocess.h) |
| tabulate | 1.5.0 | Screened | [tabulate.hpp](../../src/thirdparty/tabulate/tabulate.hpp) |
| tinyexpr | Unversioned/patched snapshot | Fixed | [tinyexpr.c](../../src/thirdparty/tinyexpr/tinyexpr.c) |
| utf8cpp | Unversioned/patched snapshot | Snapshot | [utf8.h](../../src/thirdparty/utf8cpp/utf8.h) |
| zlib | 1.3.1 | Screened | [zlib.h](../../src/thirdparty/zlib/zlib.h) |

### Nested and public-header components

| Component | Version | Location/notes |
| --- | --- | --- |
| unordered_dense | 5.0.1 | [unordered_dense.h](../../include/eepp/thirdparty/unordered_dense.h). Public container headers; stl.h and huge_page_allocator.h belong to the same import. |
| svector | 1.4.0 | [svector.h](../../include/eepp/thirdparty/svector.h). Public small-vector header. |
| stb_image | 2.30 | [stb_image.h](../../src/thirdparty/SOIL2/src/SOIL2/stb_image.h). Nested in SOIL2; older 2023 high advisories fixed; CVE-2026-5185 is medium and requires separate GIF-path follow-up. |
| stb_image_write | 1.16 | [stb_image_write.h](../../src/thirdparty/SOIL2/src/SOIL2/stb_image_write.h). Nested in SOIL2. |
| hidapi | Snapshot; parent fingerprint recorded | [hidapi.h](../../src/thirdparty/SDL2/src/hidapi/hidapi/hidapi.h). SDL-modified unversioned snapshot; update as part of SDL source refresh. |
| variant-lite | 1.2.2 | [tabulate.hpp](../../src/thirdparty/tabulate/tabulate.hpp). Nested amalgamation. |
| optional-lite | 3.2.0 | [tabulate.hpp](../../src/thirdparty/tabulate/tabulate.hpp). Nested amalgamation. |
| string-view-lite | 1.6.0 | [tabulate.hpp](../../src/thirdparty/tabulate/tabulate.hpp). Nested amalgamation. |
| minimp3-derived decoder | Snapshot; parent fingerprint recorded | [dr_mp3.h](../../src/thirdparty/dr_libs/dr_mp3.h). Covered by dr_mp3 0.7.2 fingerprint; not a separately pinned minimp3 release. |

`nlohmann/json_fwd.hpp` also exists under `include/eepp/thirdparty`; its version
macros match the implementation's 3.12.0. SOIL2's DDS/PVR/PKM/QOI/ETC helpers are
part of that submodule snapshot, not independently versioned packages. The old
SDL2 tree embeds other platform support code; updating its source should include
its nested imports rather than only the SDL headers.

Version provenance verified by direct upstream-source comparison:

- fast_float's nine implementation headers match v6.1.6.
- libogg's two C sources and two non-generated public headers match v1.3.3.
- MD4C public headers match 0.5.2/0.5.3, but implementation differs from both;
  no exact release is asserted.
- SOIL2 and efsw are pinned submodules, not ordinary copied release directories.

### Build/test-only components and external dependencies

The two generator submodules are pinned to:

- `premake/premake-ninja`: `3d214bf1fd7d2c867cdf02b4ab50fb921fdd1155`.
- `premake/premake-cmake`: `8e02bb91a4d0f29d7540de7357574cf3b7c454f9`.

`src/tests/unit_tests/utest.h` is an unversioned imported test harness, not shipped
runtime code. These development components are recorded for inventory completeness;
a full CI/toolchain vulnerability review was not performed. Doxyrest scripts under
`docs/` and development tools are likewise outside the runtime assessment.

OpenSSL, SDL2/SDL3, OS graphics/audio libraries, SSH server implementations, and
other platform packages can be selected externally. Their deployed versions cannot
be inferred from this source inventory. Audit release artifacts/package manifests
separately before declaring a shipped binary clean. Ignored downloaded SDL archives
and build outputs were not treated as the authoritative source inventory.

## Method and limits

1. Enumerated tracked vendor directories, public headers and submodules. Read
   embedded version declarations and build targets, rather than trusting directory
   names, CMake placeholder versions or API/ABI version constants.
2. Queried NVD by component names and aliases. Read upstream advisories, releases,
   issue discussions and fixes for candidate findings. Keyword/CPE matches were
   discovery inputs, not final applicability decisions. The inventory contains
   successful NVD query URLs and retrieval timestamps; a failed initial HarfBuzz
   request was retried successfully.
3. Reviewed relevant source guards, enabled backends and consumer APIs. Compared
   selected patched code with upstream fixes. High/critical means an upstream high
   label or a published CVSS score >=7; provider/metric disagreements are noted where
   they materially change interpretation. A high label does not establish eepp RCE.
4. No dependency edits, exploit execution, new build, fuzz campaign or cross-platform
   runtime validation was performed. The existing libpng archive was inspected for
   Neon symbols; it was not rebuilt. Existing test success from the earlier Brotli
   task is not evidence that these other CVEs have been tested.

Coverage gaps that remain explicit:

- Exact upstream releases/commits for unversioned modified snapshots need provenance
  work. Their local content is fingerprinted, but that does not prove which upstream
  fixes were imported.
- Database records can be incomplete, disputed or mis-scoped. "Screened" is dated
  evidence, not permanent clearance. The review does not replace code auditing or
  fuzzing for vulnerabilities without public advisories.
- Consumer API searches establish the current repository's callers. They cannot
  prove that downstream eepp users never call a bundled library directly or enable
  a different configuration.
- ARM/mobile, 32-bit, OpenSSL and other optional builds were assessed from source,
  not executed. Mbed TLS compiler-dependent issues need per-distribution analysis.

## Suggested implementation batches

1. **SSH security patch set:** transport bounds, ETM bounds, SFTP lifetime, optional
   OpenSSL bounds, plus review of the remaining upstream security fixes. Preserve
   local cryptography support. Exercise malformed-peer/error handling in isolated
   regression tests, existing SFTP tests, and both crypto backends where available.
2. **libpng update:** 1.6.59, preserving platform configuration; validate color PNG
   glyphs on ARM and the existing font/image tests.
3. **NanoSVG hardening:** establish an upstream/fork base, preserve local style/color
   support, reject non-finite/out-of-range geometry and test the reported cases.
4. **ZIP follow-up:** resolve arithmetic questions in the current fork, then decide
   between focused security backports and migration to modern libzip. Preserve
   bounded-open behavior and ownership semantics during migration.
5. **Cryptography maintenance:** Mbed TLS 3.6.7 and Monocypher 4.0.3, validating HTTP,
   certificate checks, SSH keys, session lifetime and backend compatibility.
6. **Audio/platform maintenance:** Vorbis 1.3.7 and Android SDL source reconciliation.
7. **Small exposure-reducing updates:** simdutf >=9.2.1 and PCRE2 10.49. Establish
   verified provenance for the remaining snapshot-only imports.

For subsequent updates, record the upstream release/commit and retained patches
next to the dependency, and regenerate the inventory after each batch. An automated
advisory check needs this provenance plus explicit applicability notes; a scan of
version strings alone would both miss the unversioned imports and recreate the
false positives documented above.
