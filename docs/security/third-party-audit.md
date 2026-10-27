# Third-party security audit

Last reviewed: **2026-10-27**. This document and its inventory are maintained over time; update
both whenever a dependency changes or is re-reviewed.

Initial audit: **2026-10-24**, baseline `dfc1a1941` (Brotli 1.2.1 upgrade included).
Remediation: **2026-10-25**, committed in `33dd9d621` and `7069bb03e`, with the SDL3 audio
shutdown fix `2f3f663cb`. Status refreshed **2026-10-26** against CI on `0d9acd28f`.
Scope: the 45 source trees in `src/thirdparty`, public third-party headers, nested
components, and the build configuration that selects them. This is a source inventory
and advisory/reachability review, not a deployed-binary SBOM or a claim that all code
is vulnerability-free.

The companion [machine-readable inventory](third-party-inventory.json)
now records the updated versions, upstream revision, retained patches and source
fingerprints, recomputed whenever a component changes. NVD discovery queries retain their original retrieval dates; they were
not rerun as part of this remediation. `version: null` means the exact upstream
release has not been established.

## Completed remediation — 2026-10-25

| Component | Before | Current source | Status |
| --- | --- | --- | --- |
| libssh2 | 1.11.1 plus local crypto integration | **1.11.2_DEV**, pinned to upstream `6e7b2076a6ced2230968b27ee7c1abb77fe5a88d` | Refreshed the complete imported library source, preserving eepp's build configuration, classic Mbed TLS APIs and Monocypher integration. This is an unreleased snapshot, not a 1.11.2 release. |
| libpng | 1.6.37 | **1.6.59** | Imported upstream library sources, architecture implementations and the release's prebuilt configuration without source patches. |
| NanoSVG | Modified FLTK-derived snapshot | Same snapshot plus manual security fixes | Patched the reported arc conversion, fixed-point conversion/advancement and stroke tessellation issues while retaining local style/currentColor behavior. 2026-10-26: fixed a single-stop gradient heap over-read, unbounded dashing, and NaN-to-int conversions in the rasterizer. |
| libzip | 0.10 plus local hardening | **1.12.1** | Replaced the fork with the upstream release; only an include-path patch remains. `Zip` uses the current API. ZIP browsing limits were removed. |
| Mbed TLS | 3.6.0 | **3.6.7** (LTS) | Library and headers match the release apart from three small threading patches; eepp's `mbedtls_config.h` block is retained. TLS 1.3 and PSA crypto enabled on 2026-10-27; weak algorithms remain disabled. |
| libvorbis | 1.3.6 | **1.3.7** | Includes the CVE-2018-10393 encoder bounds fix and the channel validation changes; all imported files match the release. |
| Monocypher | 4.0.2 | **4.0.3** | Fixes the compiler-dependent Ed25519 signing timing leak; imported sources match the release. |
| simdutf | 9.2.0 | **9.2.1** | Regenerated amalgamation with the same UTF-8/UTF-32-only feature set; Base64 stays disabled. |
| PCRE2 | 10.44 | **10.49** | Library sources and sljit match the release; only eepp's per-platform `config.h` is local. |
| Gumbo | codeberg `6dddad6` (0.13.2+9) | **0.14.1** | Fixes two use-after-frees, a crash and an uninitialized read in the HTML parser; sources match the release. |
| md4c | `481fbfb` (0.5.2+16) plus one local change | **0.6.0** | Unmodified; upstream now writes the same XHTML checkboxes the local change produced. |
| utf8cpp | `aa3b12c` (3.2.1+4) | **4.2.1** | Unmodified; includes the `is_valid` fix for code points above U+10FFFF. |
| HarfBuzz | 8.5.0 | **15.0.0** | Unmodified core sources; fixes for malformed fonts across seven majors. Clears the clang and MSVC warnings CI printed. |
| args | `d877e96` (6.2.2+10) | **6.6.1** | Unmodified. |
| cpp-unicodelib | `d68020d` (Unicode 16) plus MSVC patch | **18.0.0** (Unicode 18) | MSVC C++17 check reapplied. |
| stb_image (SOIL2) | 2.30 with SOIL2's GIF disposal change | 2.30 plus a local CVE-2026-5185 fix (SOIL2 `116dfde`, 2026-10-26) | Fixed a stale frame pointer in the animated-GIF loader; no upstream fix exists. |

### Validation status — 2026-10-26

CI on `0d9acd28f`: the unit tests passed on every platform that runs them, and seven of eight
workflows passed (macOS, Windows, Windows MinGW nightly, Android, iOS, Emscripten and ecode
nightly). The Linux job failed only in its NanoSVG fuzzing step, which hung on an input; that
led to the four NanoSVG fixes below, and the fuzzing step has since been removed from CI.
Only the Linux, macOS and Windows jobs run the unit tests; the others are build checks.

| Platform | Where | Result |
| --- | --- | --- |
| Linux x86_64 | CI | 2,141 of 2,141 passed, with the SFTP tests required (`EEPP_REQUIRE_SFTP_TESTS=1`) against a loopback OpenSSH server. |
| Linux x86_64 | CI, NanoSVG ASan/UBSan libFuzzer | Removed from CI after `0d9acd28f`. Earlier runs: 532,224 inputs in 61 s on `ccdc3a5d0` without findings; on `0d9acd28f` an input hung until the step's 10-minute timeout. |
| macOS | CI | 2,130 of 2,130 passed, including SFTP. |
| Windows x86_64 (MSVC) | CI | 2,062 passed; `VFSLocal.atomicWritesAndCopiesSupportLongBasenames` skipped because the runner lacks long-path support. |
| Android, iOS, Emscripten, MinGW | CI | Build only. |
| macOS ARM64 | Local, before commit | 2,128 of 2,128 passed, including SFTP and the PNG, NanoSVG, ZIP, TLS and Vorbis security tests. |
| Linux ARM64 (Ubuntu 24.04, GCC 13, SDL 3.4.10) | Local Docker, before commit | 2,139 of 2,140 passed with SFTP required; one font test skipped (no font with a non-normal stretch). |
| Windows x86_64 (MinGW-w64 GCC 13, Wine 7.7) | Local, before commit | All 21 ZIP tests passed. The full suite under Wine was inconclusive; superseded by the Windows CI run. |
| ZIP ASan/UBSan libFuzzer | Local, before commit | 300 s, 3.39 M inputs, no findings. The temporary target was then removed (see libzip). |
| macOS ARM64, NanoSVG fixes | Local, before commit | 2,133 passed; 2 audio tests skipped (no audio device). Includes the three new NanoSVG regression tests. |
| NanoSVG ASan/UBSan libFuzzer, Linux ARM64 | Local Docker, before commit | About 967,000 inputs on the fixed code with a 10 s per-input timeout, no crash, sanitizer report or slow input. Stopped by hand before its time limit. |

The first CI run of `7069bb03e` failed on Linux: the unit tests crashed with a null call in
`SDL_QuitAudio`, reached from MojoAL's `alcCloseDevice` during Engine teardown. That race
predates the dependency updates and was fixed in `2f3f663cb`.

Not run anywhere: the optional OpenSSL backend, 32-bit builds, and the unit tests on Android,
iOS and Emscripten. Passing functional tests does not establish absence of all
malformed-input vulnerabilities.

## Remaining cleanup order

| Order | Component | Current source | Action | Reason and qualification |
| --- | --- | --- | --- | --- |
| 1 | Bundled SDL2 source | 2.0.12 | Remove the vendored copy; download the pinned SDL release before the Android build, as the MinGW and MSVC builds do, and port Android to SDL3 (recommended) | Old source has high-severity surface/blit issues; an application exploit path was not established. Its local changes are assessed below. |

Orders indicate engineering priority for this repository, not a new CVSS score.
Targets are releases verified during the initial audit; check upstream again before
performing the remaining upgrades.

## Resolved findings and retained integration

### libssh2

Upstream snapshot:
[6e7b2076](https://github.com/libssh2/libssh2/commit/6e7b2076a6ced2230968b27ee7c1abb77fe5a88d),
2026-10-24. The complete library refresh includes the fixes below and later upstream
security fixes, rather than only applying the original audit's six patch candidates.
[`README.eepp`](../../src/thirdparty/libssh2/README.eepp) records import provenance and
all retained integration points.

| Advisory | Initial finding / severity | Remediation |
| --- | --- | --- |
| [CVE-2026-55200](https://nvd.nist.gov/vuln/detail/CVE-2026-55200) | Critical, CNA CVSS 4.0 9.2; missing full-packet upper bound, including ChaCha20 use | Upstream transport bounds fix incorporated. |
| [CVE-2026-66035](https://nvd.nist.gov/vuln/detail/CVE-2026-66035) | High, 7.5; ETM decrypt allocation/copy bounds | Upstream minimum-size checks and subsequent error-path cleanup incorporated. |
| [CVE-2026-66032](https://nvd.nist.gov/vuln/detail/CVE-2026-66032) | High, 8.8; SFTP open error-path double free | Upstream dangling-pointer fix incorporated. |
| [CVE-2026-66033](https://nvd.nist.gov/vuln/detail/CVE-2026-66033) | High, 7.5; optional OpenSSL AES-GCM bounds | Upstream fix incorporated; optional backend not built in this validation. |
| [CVE-2026-66034](https://nvd.nist.gov/vuln/detail/CVE-2026-66034), [CVE-2026-58050](https://nvd.nist.gov/vuln/detail/CVE-2026-58050) | High, 7.5 / 7.0; publickey-list parsing and 32-bit attribute-count arithmetic | Upstream fixes incorporated even though no application consumer of this subsystem was found. |
| [CVE-2026-7598](https://nvd.nist.gov/vuln/detail/CVE-2026-7598) | Provider scores differ; username/password size arithmetic | Upstream authentication input checks incorporated. |

Verified fixes from the initial audit remain useful provenance:
[packet upper bound](https://github.com/libssh2/libssh2/commit/97acf3dfda80c91c3a8c9f2372546301d4a1a7a8),
[ETM bounds](https://github.com/libssh2/libssh2/commit/42e33d81577ed4b95d4b4f6f845e5ee8efe5eeb4),
[SFTP lifetime](https://github.com/libssh2/libssh2/commit/5e4776146552d898b9c0e1b313cd093fa8dc92d0),
[OpenSSL GCM bounds](https://github.com/libssh2/libssh2/commit/a2ed82d40964bbc0d64cd717aa0a5a892117d2e6),
[publickey bounds](https://github.com/libssh2/libssh2/commit/a13bb6c773f0d55ad1628cede57e99803cd898d9),
[authentication lengths](https://github.com/libssh2/libssh2/commit/256d04b60d80bf1190e96b0ad1e91b2174d744b1).

The only modified upstream implementation files are `src/mbedtls.c` and
`src/mbedtls.h`. Upstream moved hashing/HMAC/random generation to PSA, which eepp
explicitly disables. The local adapter retains the prior classic Mbed TLS digest,
HMAC and CTR-DRBG APIs without enabling global PSA state or changing Mbed TLS
configuration. Existing per-operation allocation behavior is retained; failed hash
initialization is cleaned up and digest output sizes are checked.

The eepp-owned `mbedtls_ed25519.c` was adapted to upstream's private names,
unified file/blob key parser and NUL-terminated method strings. It retains key
validation, private-key wiping, rejection of all-zero X25519 shared secrets and
Monocypher-backed Ed25519/X25519 support. The static platform configuration is
retained. New upstream backend files compile independently rather than through
the removed `crypto.c` umbrella.

Current upstream defaults disable legacy SHA1/MD5/DSA algorithms. Servers supporting
only those methods may fail negotiation; weak algorithms were not re-enabled.
Host-key validation remains necessary and is exercised by the SFTP suite.

### libpng

Updated from 1.6.37 to [1.6.59](https://www.libpng.org/pub/png/libpng.html).
All imported library source files match upstream, and `pnglibconf.h` is the release's
`scripts/pnglibconf.h.prebuilt`. The old local `math.h` portability change is obsolete
in the newer source. New architecture implementations are included by the existing
recursive Premake source glob.

[CVE-2026-33636 / GHSA-wjr5-c57x-95m2](https://github.com/pnggroup/libpng/security/advisories/GHSA-wjr5-c57x-95m2)
affected our ARM Neon palette expansion and was fixed upstream in 1.6.56. The new
release also includes subsequent security fixes. FreeType's PNG color glyph loader
is the relevant consumer; ordinary SOIL2 PNG image loading uses stb_image instead.

The upgrade also covers the initial audit's conditional matches CVE-2026-33416,
CVE-2026-25646, CVE-2025-64720, CVE-2025-65018, CVE-2025-66293 and CVE-2026-22801.
Their API preconditions were not established in our wrapper, but no separate old
implementation is retained. CVE-2026-22695 was introduced after our old 1.6.37 and
was not an original exposure.

[`png_security_tests.cpp`](../../src/tests/unit_tests/png_security_tests.cpp) decodes
independently generated 17-by-2 palette PNGs with and without transparency and
checks every output channel. This exercises partial Neon chunks on ARM64 and runs
against scalar/SIMD implementations selected on other hosts. The test target links
libpng and zlib explicitly on both Premake versions so Windows does not depend on
private libpng symbols being exported by `eepp.dll`.

### NanoSVG

The modified FLTK-derived base and local style/currentColor behavior are preserved.
Exact upstream base provenance remains unresolved; these fixes do not invent a new
upstream release number. [`README.eepp`](../../src/thirdparty/nanosvg/README.eepp)
records the manual changes.

| Advisory | Applied change |
| --- | --- |
| [CVE-2026-88366 / upstream #294](https://github.com/memononen/nanosvg/issues/294) | Reject non-finite arc endpoints; treat invalid radii/rotation or non-finite intermediate angles/centers as a line before the angle-to-int cast. |
| [CVE-2026-88368 / upstream #292](https://github.com/memononen/nanosvg/issues/292) | Validate finite edge geometry and rounded fixed-point values before casting or consuming a pool entry. Compare int bounds in double to avoid `float(INT_MAX)` rounding. Reject overflowing scanline advancement and continue processing subsequent valid edges. |
| [CVE-2026-88367 / upstream #293](https://github.com/memononen/nanosvg/issues/293) | Validate stroke tessellation inputs, compute using double intermediates and cap subdivisions at 1024 before integer conversion. |

Extreme malformed geometry may be omitted or approximated. The 2026-10-25 fixes add no
rasterizer allocations and do not change the public or internal structure layouts.
[`nanosvg_security_tests.cpp`](../../src/tests/unit_tests/nanosvg_security_tests.cpp)
covers infinite/large finite arc radii, invalid endpoints, bounded stroke subdivision,
range/NaN edge rejection without pool consumption, both directions of scanline
integer overflow and continued rendering of styled/currentColor shapes.

These findings concern numeric undefined behavior and availability. The original
reports include sanitizer-triggered termination; neither the initial audit nor this
remediation establishes an eepp RCE or a deterministic release-build crash.

#### Rasterizer fixes found by fuzzing — 2026-10-26

The CI fuzzing step on `0d9acd28f` hung on an input; investigating it found four bugs in
`nanosvgrast.h`, all reachable from SVG input. No public advisories cover them.

| Issue | Evidence | Fix |
| --- | --- | --- |
| A gradient with a single stop built its 256-entry color ramp from `stops[0..255]`, reading up to 255 stops past the allocation. Single-stop gradients are valid SVG, so ordinary files trigger it. | ASan heap-buffer-overflow in `nsvg__initPaint` on a one-stop `<linearGradient>` fill. | Fill the ramp from `stops[0]`. |
| Dashed strokes had no bound. A long path with a tiny dash pattern emitted an unbounded number of dashes, and once a dash was below the float precision of the coordinates the dash position stopped advancing, so the loop never ended. | The CI hang. A valid 1e9-unit line with a 1-unit dash ran past a 60 s limit. | Cap dash segments at 16,384 per `nsvgRasterizeXY` call (new `NSVGrasterizer::dashSegments` counter). The cap is per image because dashes on shared scanlines make rasterization quadratic; at the cap the worst case is about 150 ms in release builds. Dashes past the budget are not drawn. |
| `nsvg__clampf` returned NaN unchanged; callers convert the result to `int`, including as a gradient color-ramp index. | Code review. | NaN clamps to the lower bound. |
| `nsvg__roundJoin` converted a NaN segment count to `int` when point deltas were non-finite. | UBSan report on a fuzzer-generated input. | Fall back to the maximum segment count. |

The dash counter is the only layout change, in the rasterizer's internal struct; the public API
and allocations are unchanged. [`nanosvg_security_tests.cpp`](../../src/tests/unit_tests/nanosvg_security_tests.cpp)
adds tests for the single-stop ramp, NaN clamping and the shared dash budget. On the fixed
code the one-stop gradient, the saved UBSan input, the long dashed line and a 40-line dashed SVG
run without sanitizer reports, and about 967,000 fuzzer inputs ran without findings. The fuzz
target is kept for manual runs (`--with-security-fuzz`) but is no longer run in CI.

### libzip

Replaced the 0.10 fork with [libzip 1.12.1](https://github.com/nih-at/libzip/releases/tag/v1.12.1)
(`libzip-1.12.1.tar.xz`, SHA-256
`c40062022a1963c80573d48f6070792297cecd7c6bffe3aefcac0d8b9bee8913`, matching the GitHub
release digest). [`README.eepp.md`](../../src/thirdparty/libzip/README.eepp.md) records the
imported sources, the hand-written `config.h`/`zipconf.h` that replace CMake, the generated
error table and the retained patches.

The initial audit's open questions about the old code no longer apply:

- The `cd->offset + cd->size` addition (CVE-2012-1163 family) and the unchecked
  `_zip_cdir_new()`/`_zip_cdir_grow()` multiplications (CVE-2015-2331 family) are gone. 1.12.1
  reads 64-bit EOCD fields through bounds-checked buffers, rejects wrapping offsets and sizes,
  rejects ZIP64 entry counts larger than the directory can hold, and grows arrays through
  overflow-checked `ZIP_REALLOC`.
- The signed-shift undefined behavior that a sanitizer run found in the old `_zip_read4()`, and the
  entry-capacity update made before `realloc()` succeeded, existed only in the old fork. No
  uncommitted fixes to 0.10 were kept.
- CVE-2017-12858 and CVE-2017-14107 are fixed upstream. 1.12.1 also includes the later
  hardening listed in the [security history](https://libzip.org/security/).

The only source patch is `zip.h` including `"zipconf.h"` instead of `<zipconf.h>`.

The old fork's `ZIP_LIMIT_METADATA` bounded open, and the VFS `ZipProvider` limits of 262,144
entries and a 64 MiB metadata budget, were removed at the maintainer's direction. ZIP browsing
now accepts whatever the format and libzip support. libzip bounds the entry count by the
central directory size, so index memory grows with the archive: about 300 bytes per typical
entry, and roughly 10× the directory size for a crafted directory of empty extra fields.
`ArchiveProvider` (libarchive) still has its own 262,144-entry / 64 MiB limits.

`Zip` now opens read-only mode with `zip_open(ZIP_RDONLY)`. libzip converts UTF-8 paths to
wide paths on Windows, so the `fopenUtf8`/`dup`/`zip_fdopen` ownership workaround was removed,
and `ReadWrite` opens gain Unicode support on Windows. Deprecated calls were replaced by
`zip_file_add`, `zip_get_error`, `zip_error_strerror` and `zip_discard`. The old 0.10
Windows `rename()` workaround is superseded by upstream's Win32 backend. bzip2, xz, zstd and
WinZip AES remain disabled, as they were in 0.10. On Windows, `advapi32` is now linked for
upstream's random-number and ACL code.

InfoZIP archives whose 16-bit count wraps (for example, exactly 65,536 entries) now open,
and libzip recovers the count from the directory size; the old fork rejected them because it
could not decode them safely.

Validation:
[`zip_security_tests.cpp`](../../src/tests/unit_tests/zip_security_tests.cpp) covers wrapped and
high-bit 32-bit ranges, 64-bit offset/size wrapping, and ZIP64 counts at 2^32 and 2^32−1 (which
wrap a 32-bit allocation). A 300 s ASan/UBSan libFuzzer run of a temporary ZIP target, through
the public `zip_open_from_source()` API, executed 3.39 M inputs without a finding. That target was
then removed: libzip is now unmodified apart from one include line, and upstream maintains
OSS-Fuzz harnesses. The NanoSVG target remains, because the local fork has no upstream fuzzing;
it is built with `--with-security-fuzz` for manual runs and is not part of CI.

The ZIP and ZIP-provider tests pass on every platform in the validation table above:
Linux x86_64, macOS and Windows (MSVC) in CI, and locally on macOS ARM64, Linux ARM64
and under Wine for MinGW. The Android, iOS and Emscripten CI jobs build the new libzip
configuration.

### Mbed TLS

Updated from 3.6.0 to the [3.6.7 LTS release](https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7)
(`mbedtls-3.6.7.tar.bz2`, SHA-256
`a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6`), avoiding a simultaneous
4.x migration. Every imported library source and header matches the release except
`include/mbedtls/mbedtls_config.h`, whose trailing eepp block is retained (weak key exchanges,
DES and DHM disabled, deprecated APIs removed, AESCE limited to AArch64 Linux), and the threading
patches described under "TLS 1.3" below. The release's PSA sources are present again (3.6.0's
import had deleted them as unused) and are used since TLS 1.3 was enabled. The libssh2
classic-API adapter is unchanged.

The ordinary [`MbedTLSSocket`](../../src/eepp/network/ssl/backend/mbedtls/mbedtlssocket.cpp)
is a stream TLS client. It sets the hostname and uses certificate verification when requested;
it disables TLS 1.2 session tickets and, for TLS 1.3, only receives tickets to resume its own
sessions. Its client role excludes several high findings. The following is a triage of high upstream or high NVD
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
| CVE-2026-50713 | Introduced in 3.6.3, after the old 3.6.0, and fixed in 3.6.7 (DTLS renegotiation with `badmac_limit`). DTLS also differs from the eepp stream TLS path. |

Advisory definitions and fixes: [official security index](https://mbed-tls.readthedocs.io/en/latest/security-advisories/).
The individual documents were read from the official `Mbed-TLS/mbedtls-docs`
source archive, including issues whose NVD descriptions omit configuration details.
The [AESNI advisory](https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2025-06-1/)
is particularly useful when assessing x86 distribution builds. 3.6.7 also clears the low and
medium fixes accumulated since 3.6.0.

#### TLS 1.3 — 2026-10-27

Both TLS backends now negotiate TLS 1.3, keeping TLS 1.2. In Mbed TLS 3.6, TLS 1.3 runs on PSA
crypto: `MBEDTLS_PSA_CRYPTO_C` and `MBEDTLS_SSL_PROTO_TLS1_3` are no longer undefined, while
`MBEDTLS_PSA_CRYPTO_CONFIG`, persistent key storage (`MBEDTLS_PSA_CRYPTO_STORAGE_C`,
`MBEDTLS_PSA_ITS_FILE_C`), LMS and `MBEDTLS_USE_PSA_CRYPTO` stay off, so TLS 1.2 and libssh2 keep
their classic implementations. `MbedTLSSocket::init()` calls `psa_crypto_init()` and `end()` calls
`mbedtls_psa_crypto_free()`; `SSLSocket` defers `end()` until no socket is alive.

Mbed TLS requires `MBEDTLS_THREADING_C` for TLS 1.3 in multithreaded programs, because threads
with separate TLS contexts still share PSA's key store. POSIX builds use
`MBEDTLS_THREADING_PTHREAD`. Mbed TLS has no Windows implementation, so eepp adds
`MBEDTLS_THREADING_WIN32`, small local patches that keep every caller unchanged:

| File | Local change |
| --- | --- |
| `include/mbedtls/check_config.h` | Accepts `MBEDTLS_THREADING_WIN32` as the single threading implementation (Windows only) |
| `include/mbedtls/threading.h` | Mutex type holding an `SRWLOCK` as `void *`, so the public header does not include `<windows.h>` |
| `library/threading.c` | `SRWLOCK` init/lock/unlock and statically initialized global mutexes (`MUTEX_INIT = { NULL, 1 }`), like the pthread path, so nothing must run before the library is first used |

Session resumption, which FTPS data connections rely on (`SSLSocket`'s restore-session
constructor; servers such as vsftpd with `require_ssl_reuse` reject data connections that do not
resume), is implemented for TLS 1.3 in both backends. TLS 1.3 resumes only from NewSessionTicket
messages, sent after the handshake:

- `MbedTLSSocket` enables `mbedtls_ssl_conf_tls13_enable_signal_new_session_tickets` and keeps the
  latest session whenever a ticket arrives. A ticket is first reported as `WANT_READ` while Mbed
  TLS holds it buffered, so `receive()` processes it at once (`mbedtls_ssl_check_pending()`)
  instead of returning `NotReady`.
- `OpenSSLSocket` never restored sessions, for any TLS version; FTPS against servers requiring
  reuse could not work with it. It now keeps every session OpenSSL issues through a client
  session-cache callback and offers the latest with `SSL_set_session()`.
- In both backends a socket keeps its session after its connection closes, and releases it when
  it reconnects or is destroyed; restoring sockets copy it under a mutex.

[`tls_security_tests.cpp`](../../src/tests/unit_tests/tls_security_tests.cpp) runs loopback
OpenSSL `s_server` instances restricted to TLS 1.2 and to TLS 1.3. It checks the negotiated
version, accepts a trusted certificate, and rejects a wrong host name and an untrusted
certificate; it also checks that a restored session is reported as resumed (`Reused, TLSv1.x`)
on both versions.

Validation: macOS ARM64 with Mbed TLS, full suite (2,134 passed, 2 audio tests skipped); Linux
ARM64 (Docker) with Mbed TLS, full suite (2,146 passed, one font test skipped) including the 35
SFTP tests required; Linux ARM64 with the OpenSSL backend (OpenSSL 3.0.13), the TLS tests; MinGW-w64
builds Mbed TLS and libssh2 with the `SRWLOCK` mutexes. eepp's HTTP client fetched google.com,
github.com and cloudflare.com. MSVC is left to CI.

Building with `--with-openssl` also needed `libcrypto` linked wherever libssh2 is, since libssh2's
OpenSSL backend calls it directly; the unit tests did not link before.

### libvorbis

Updated from 1.3.6 to [1.3.7](https://github.com/xiph/vorbis/blob/v1.3.7/CHANGES)
(`libvorbis-1.3.7.tar.xz`, SHA-256
`b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b`). All imported files match
the release, including the CVE-2018-10393 loop/index bounds in `lib/psy.c` and the
CVE-2018-10392 channel validation. The standalone tool sources that eepp removed earlier
(`tone.c`, `barkmel.c`) remain excluded. [`SoundFileWriterOgg`](../../src/eepp/audio/soundfilewriterogg.cpp)
drives the encoder; [`vorbis_security_tests.cpp`](../../src/tests/unit_tests/vorbis_security_tests.cpp)
encodes and decodes mono and stereo at 1 Hz to 44.1 kHz and checks that invalid rates and
channel counts are rejected.

### stb_image GIF loader (CVE-2026-5185)

[CVE-2026-5185](https://nvd.nist.gov/vuln/detail/CVE-2026-5185) is a medium (CVSS 3.1 5.3)
heap memory-safety bug in `stbi__gif_load_next`, tracked upstream as
[nothings/stb #1916](https://github.com/nothings/stb/issues/1916). Upstream has no fix. In
upstream 2.30 the cause is the `two_back` pointer computed before the start of the frame buffer.

SOIL2's copy had already replaced `two_back` (SOIL2 `1acee65`, "Fix gif dispose logic") with a
`last_non_disposable` pointer. It was set to the start of the accumulated frame buffer and refreshed
only after frames with disposal method 0 or 1. After a frame with disposal 2 or 3, the `realloc`
that appends it could move the buffer, so a later disposal-3 frame read through a freed pointer.

eepp reaches this path: `Image::loadGif` (used by `UIImageViewer`, e.g. the ecode and eefm
previews) and `Texture::loadGif` / `Sprite::fromGif` call `stbi_load_gif_from_memory`. Opening a
crafted animated GIF was enough; no network route is needed.

Fixed in SOIL2 [`116dfde`](https://github.com/SpartanJ/SOIL2/commit/116dfde76de4ebac11531c98dfb45e4fae400948):
`stbi__load_gif_main` stores the frame index and rebuilds the pointer from the current buffer
before each frame. Disposal 3 now restores the most recent frame with disposal 0 or 1 instead of
always frame 0. The rest of SOIL2's customized stb_image, including its added formats, is
unchanged. eepp's submodule points to this commit.

Validation: five real animated GIFs, covering disposal methods 1, 2 and 3 (up to 321 frames),
decode identically with the old and patched loaders under ASan/UBSan with no reports, and the
macOS release suite passes (2,128 passed, 2 audio tests skipped for lack of an audio device). None
of those files contains the frame sequence that triggered the bug; no crafted reproducer was built.

### PCRE2

Updated from 10.44 to [10.49](https://github.com/PCRE2Project/pcre2/releases/tag/pcre2-10.49)
(`pcre2-10.49.tar.bz2`, SHA-256
`53c156e1ba416a20da8e65395daa132da0d80e76910424caca3fcdae7831d384`). The library sources,
`pcre2.h` (from `pcre2.h.generic`), `pcre2_chartables.c` (from `.dist`) and `deps/sljit` match
the release; the tree now follows upstream's `src/` + `deps/sljit/` layout, so the JIT's
relative include works unmodified. eepp's per-platform `src/config.h` is retained, minus the
`HAVE_MEMMOVE`/`HAVE_STRERROR` checks that 10.49 no longer reads, and keeps its stricter
`MAX_NAME_SIZE` (32) and `MAX_VARLOOKBEHIND` (64). 10.49 adds `pcre2_compile_cgroup.c`,
`pcre2_compile_class.c` and `pcre2_match_next.c` to the Premake and Android source lists.

The advisories below were not reachable through eepp's wrapper before the update; they are
kept for reference:

| Advisory | Why ordinary eepp Regex use differs |
| --- | --- |
| CVE-2026-103111 / GHSA-r9hj-j2rw-4q3m | Requires explicitly created/assigned growable JIT stack; no consumer `pcre2_jit_stack_create` or `pcre2_jit_stack_assign` calls found. |
| CVE-2026-86145 / GHSA-3r4p-g7gg-ppmf | Requires DFA matching; no consumer `pcre2_dfa_match` calls found. |
| CVE-2026-89161 | Requires direct JIT match with previously copied subject state; wrapper calls `pcre2_match`, creates fresh match data, and does not request `PCRE2_COPY_MATCHED_SUBJECT`. |
| CVE-2026-89157 | Requires large pattern conversion on 32-bit systems; no consumer pattern-convert calls found. |
| CVE-2025-58050 | Only 10.45; our 10.44 is outside the affected interval. |

Sources: [10.49 advisory](https://github.com/PCRE2Project/pcre2/security/advisories/GHSA-r9hj-j2rw-4q3m),
[DFA advisory](https://github.com/PCRE2Project/pcre2/security/advisories/GHSA-3r4p-g7gg-ppmf),
[JIT state fix](https://github.com/PCRE2Project/pcre2/pull/937),
[supported backports](https://github.com/PCRE2Project/pcre2/blob/main/SUPPORT-LIFECYCLE.md).

### Gumbo, md4c and utf8cpp

These three were unversioned imports. Comparing every upstream commit with eepp's copy
established their bases, and each was then moved to its current release:

| Library | Established base | Local changes | Now |
| --- | --- | --- | --- |
| Gumbo | [codeberg](https://codeberg.org/gumbo-parser/gumbo-parser) `6dddad6` (0.13.2+9, 2025-10-11), the maintained continuation of Google's archived repository | None in the sources | 0.14.1 (`33103d8`) |
| md4c | `481fbfb` (0.5.2+16, 2024-02-25) | `md4c-html.c` wrote task-list checkboxes as XHTML | 0.6.0 (`7fc1815`) |
| utf8cpp | `aa3b12c` (3.2.1+4, 2022-10-17) | None | 4.2.1 (`3d1ff6d`) |

Gumbo is a security update. 0.14.0/0.14.1 fix two use-after-frees and an empty-stack crash in
`selectedcontent` handling and an uninitialized quirks-mode read. eepp's copy contained that
code, and the HTML compatibility layer parses untrusted pages with it. 0.14.1 adds the
`GUMBO_NODE_PROCESSING_INSTRUCTION` node type, which `HTMLFormatter` now skips like comments.
Three upstream warnings remain for variables used only by `assert()`.

md4c 0.6.0 emits `disabled="true"`, `checked="true"` and ` />` under `MD_HTML_FLAG_XHTML`, the
flag `Markdown::toXHTML` passes, so the local change is no longer needed; rendering a task list
produces byte-identical output. utf8cpp 4.2.1 makes `is_valid` reject code points above
U+10FFFF; eepp's other two call sites use `unchecked::next`.

`subprocess` is an eepp-maintained fork that has diverged substantially from
[sheredom/subprocess.h](https://github.com/sheredom/subprocess.h); it has no upstream release
to track and is reviewed as eepp code.

Validation: the macOS release build of the unit tests and ecode is warning-free apart from the
Gumbo `assert()` variables, and the full suite passes (2,128 passed, 2 audio tests skipped for
lack of an audio device), including 322 HTML, Markdown and web-view tests and the regex tests
with the JIT enabled.

### Provenance of the remaining unversioned imports

Matching each import against every upstream commit established its base, and whitespace-insensitive
diffs separated real local changes from line-ending noise:

| Import | Base | Local changes | Action |
| --- | --- | --- | --- |
| args | `d877e96` (6.2.2+10, 2019-08-18) | None | Updated to 6.6.1 |
| cpp-unicodelib | `d68020d` (Unicode 16.0.0, 2024-10-13) | C++17 check also accepts `_MSVC_LANG` (eepp does not build with `/Zc:__cplusplus`) | Updated to v18.0.0 (Unicode 18.0.0); patch reapplied |
| backward-cpp | `0bfd0a0` (v1.6+70, 2025-04-14) | Demangling and symbol names on Windows with GCC/MinGW, which upstream does not support | eepp-maintained fork |
| portable-file-dialogs | `7f852d8` (0.1.0+67, 2022-11-11) | Windows owner window, save-dialog extension and flags, macOS save location/name, `_WIN32_WINNT` and POSIX/BSD macros | eepp-maintained fork; upstream is largely unmaintained |
| jpeg-compressor | `aeb7d3b` (2020-05-07), upstream's latest | Drops `#include <malloc.h>` | None needed |
| fts_fuzzy_match | `d541eb4` (2017-02-18), the file's latest | `[[maybe_unused]]` on three declarations | None needed |
| emscripten-browser-clipboard | `633dc92` (2023-03-09) | Ctrl/Cmd+V and right-click paste for SDL2's Emscripten backend | eepp-maintained fork; upstream's later refactors are not tracked |

backward-cpp, portable-file-dialogs and emscripten-browser-clipboard, like subprocess, are now
eepp-maintained forks: their upstream bases are recorded for reference, but upstream is no longer
tracked and they are reviewed as eepp code.

The initial audit's NVD screening found no actionable advisories for these imports; it was not rerun in this pass. Validation: the macOS release build of
all targets compiles them without warnings; the full suite passes (2,133 passed, 2 audio tests skipped),
including 65 string and Unicode-boundary tests; `ecode`, `eterm`, `eefm` and `eproc` print `--help` and
reject an unknown flag with args 6.6.1.

## High findings whose current eepp API conditions were not found

### HarfBuzz

Updated from 8.5.0 to [15.0.0](https://github.com/harfbuzz/harfbuzz/releases/tag/15.0.0)
(`harfbuzz-15.0.0.tar.xz`, SHA-256
`968536e9abd782f07f7954044c0c4020199fcdd39b41d221ec42d2a3be4a44df`). eepp's copy of 8.5.0 had
no local source edits; only `config.h` is local, and it is retained unchanged. Fonts are
untrusted input (the HTML layer can load them from pages), and the releases since 8.5.0 fix many
malformed-font issues, including a use-after-free in `hb_font_set_face()`, a heap-buffer-overflow
in `hb_font_set_variation()`, signed and float-to-int overflows in glyph scaling and variation
data, and crashes and hangs on malformed fonts. eepp only uses core shaping and the FreeType
integration (`hb_shape_full`, `hb_buffer_*`, `hb_ft_*`).

The vendored tree is now upstream's `src/` restricted to the core library: the headers, `OT/`,
`harfbuzz.cc` and the 69 sources it includes. The subsetter (`hb-subset*`, `graph/`), Cairo,
ICU, GLib, GDI/DirectWrite/CoreText/Uniscribe backends, Rust/WASM samples and generator
scripts are no longer copied; eepp used none of them, and the platform backends were already
compiled out by their `HAVE_*` guards. The 8.5.0 advisories in the Cairo integration
(CVE-2024-56732) and the experimental WASM shaper (CVE-2026-113679) no longer apply: Cairo is not
vendored, and WASM stays disabled (`HAVE_WASM` is not defined).

Premake builds HarfBuzz from the separate sources when the generating machine has 8 or more CPU
threads, and from upstream's single `harfbuzz.cc` otherwise: the single unit does about a quarter
of the total compile work (14 s on one core against 54 core-seconds measured on Apple M-series),
so it is faster on 2-4-core CI runners, while separate sources finish sooner on many-core
machines. ndk-build always uses `harfbuzz.cc`.

The update also clears the HarfBuzz warnings CI printed: clang 20's `-Wnontrivial-memcall`
(283 per build on macOS, iOS and Emscripten, all from one `memcpy` in `hb-null.hh`, now cast to
`void*` upstream) and MSVC `C5105` (1,976, from `hb_has_builtin` expanding to `defined` under
`/Zc:preprocessor`; 15.0.0 defines it as `0` for MSVC). HarfBuzz compiles without warnings with
macOS clang 17 in both modes and with Linux GCC 13 and MinGW-w64 GCC 13 in single-unit mode;
clang 20+ and MSVC are confirmed by CI.

Validation: macOS release suite with each build mode, 2,133 passed and 2 audio tests skipped
(no audio device), including 391 font, text, shaping and RTL tests.
Sources: [HarfBuzz releases](https://github.com/harfbuzz/harfbuzz/releases),
[HarfBuzz security](https://github.com/harfbuzz/harfbuzz/security).

### simdutf and Monocypher

simdutf was updated from 9.2.0 to 9.2.1, which fixes the high
[GHSA-pw55-7mm7-cvhh](https://github.com/simdutf/simdutf/security/advisories/GHSA-pw55-7mm7-cvhh)
Base64 issue and the related LASX Latin-1 issue. eepp vendors a regenerated amalgamation
limited to UTF-8 and UTF-32, as before; Base64 and Latin-1 support are not compiled, so
neither advisory was reachable.

Monocypher was updated from 4.0.2 to 4.0.3, which fixes the compiler-dependent EdDSA/Ed25519
timing leak. Our SSH integration signs with Ed25519, so this was relevant. The imported
`monocypher.{c,h}` and `monocypher-ed25519.{c,h}` match the release. Sources:
[known bugs](https://monocypher.org/bugs),
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

#### Android SDL2 local changes — assessed 2026-10-26

Compared with the `SDL2-2.0.12.tar.gz` release (SHA-256
`349268f695c02efbc9b9148a70b85e58cefbbf704abd3e91be654db7f1e2c863`), 900 of the 901 files in
`src/thirdparty/SDL2` are identical. The Android build's SDL pieces carry three local changes:

| Where | Local change | In SDL2 2.32.10 | In SDL3 3.4.10 |
| --- | --- | --- | --- |
| `src/thirdparty/SDL2/src/sensor/android/SDL_androidsensor.c` | `ALooper_pollAll` → `ALooper_pollOnce`; newer NDKs reject `pollAll` | Upstream | Upstream |
| `projects/android-project/.../org/libsdl/app/HIDDeviceManager.java` | `PendingIntent.FLAG_MUTABLE` on Android 12+ | Upstream | Upstream |
| `projects/android-project/.../org/libsdl/app/SDLActivity.java` | `getLibraries()` loads `BuildConfig.SO_NAME` instead of `"main"`, so each app flavor loads its own library | Still needed | Still needed |

The SDL2 module in `projects/android-project/app/jni/eepp.mk` is 2.0.12's `Android.mk` unchanged
apart from `LOCAL_PATH := $(SDL_PATH)`. 2.32.10's `Android.mk` adds `audio/aaudio`,
`joystick/dummy`, `joystick/virtual`, `locale`, `misc` and their Android sources, builds hidapi
into SDL2 (no separate `libhidapi` module) and adds `SDLSurface.java`. SDL3 3.4.10
(`SDL3-3.4.10.zip`, SHA-256 `be8dafef3ecc956e736cfd1c242c10737f3ad589a866f25ba437b7f2f6d5ced2`)
still ships an `Android.mk` (module `SDL3`) and its Java classes; its sample project uses
`minSdkVersion 21` and `compileSdkVersion 35` (eepp: `minSdk 27`, `compileSdk 34`).

Plan: remove `src/thirdparty/SDL2`. A Gradle `preBuild` task downloads the pinned SDL release into
`src/thirdparty/SDL3-<version>`, the directory premake already uses and `.gitignore` already
excludes, taking the version from the Premake pin. `eepp.mk` includes SDL's own `Android.mk`
instead of a copy and builds the SDL3 backend and mojoAL-SDL3; SDL3's main is header-only, so the
`src/main/android/*.c` entry goes. The library-name change moves into a small `SDLActivity`
subclass so SDL's Java classes stay unmodified. Porting directly to SDL3 is recommended over SDL2
2.32.10 first: the desktop default backend is already SDL3 and the SDL3 backend already has the
same Android branches as SDL2's, so an SDL2 step would redo the Java, build-file and device-testing
work. eepp on SDL3 has not run on Android yet, so input, text input and lifecycle need device testing.

## Important exclusions and already-fixed issues

| Match | Reason it is not an outstanding high-CVE upgrade finding here |
| --- | --- |
| Brotli CVE-2026-111497 | Already upgraded to 1.2.1 in baseline commit `dfc1a1941`. |
| tinyexpr CVE-2026-88355 | `new_expr()` already allocates `sizeof(te_expr) + extra`, never less than `sizeof(te_expr)`. Local code predates and independently prevents the reported condition. [Original issue](https://github.com/codeplea/tinyexpr/issues/145). |
| FreeType CVE-2025-27363 / CVE-2026-23865 | Our 2.14.1 is newer than the affected versions. Pillow's CVE-2026-112893 is a caller bug in a different project. FreeType's dependent codecs remain separately covered above. |
| libwebp CVE-2023-4863 | Our 1.5.0 includes the fix shipped in 1.3.2. Later matches about Perl Image::WebP's bundled old decoder or an npm wrapper are different products. |
| stb_image's 2023 high advisories | SOIL2 bundles 2.30, beyond those affected releases. The medium GIF finding CVE-2026-5185 is fixed locally (see above); do not label stb_image vulnerability-free. |
| dr_libs WAV CVEs | `dr_wav.h` is not vendored or used. We have dr_flac and dr_mp3. Their metadata and whole-file APIs must be assessed independently; WAV advisories are not transferable. |
| zlib CVE-2026-22184 | `contrib/untgz` demonstration utility is absent, not part of the built library. |
| zlib CVE-2026-27820 | Ruby `zlib` binding, not native zlib, despite a misleading broad version/CPE match. |
| zlib CVE-2026-85091 | Concerns non-blocking `gzwrite`/`gzprintf` handling. Our 1.3.1 predates the described regression and no consumer use was found. Older-version scope was still being discussed; if upgrading to 1.3.2, include the subsequent upstream fix rather than blindly replacing 1.3.1. |
| zlib CVE-2026-27171 | CRC combine negative-length issue is below high severity; no affected application API use found. |
| libyaml 2024 emitter/document reports | Current application use is initialized parser/event processing. Several reports concern API misuse or were rejected; do not infer a high parser exposure from the 0.2.5 number. [Maintainer analysis](https://github.com/yaml/libyaml/issues/303). |
| jpeg-compressor CVE-2018-13030 / CVE-2018-13037 | Target `stb_image.c`, which is absent from this import; current image decoder is separately inventoried as stb_image 2.30. |
| Gumbo CVE-2025-6490 | Withdrawn report about Nokogiri fork's `hashmap.c`; the codeberg Gumbo tree eepp uses has no such file. |
| Chipmunk CMS CVEs | Unrelated PHP applications, not Chipmunk2D physics. |
| nlohmann CVE-2024-34363 | Envoy's handling of exceptions, not a nlohmann library version vulnerability. |

## Complete source-tree inventory

`Screened` means no actionable matching high/critical issue was established in this
pass. It does not mean no bugs or no lower-severity advisories exist.
`Snapshot` means upstream release provenance is incomplete; retain the fingerprint
and establish a verified tag/commit before calling a future update routine.
`Owned` means an eepp-maintained fork with no upstream release to track.

| Component | Version established | Disposition | Local evidence |
| --- | --- | --- | --- |
| args | 6.6.1 | Screened | [args.hxx](../../src/thirdparty/args/args.hxx) |
| backward-cpp | eepp-maintained fork (from `0bfd0a0`) | Owned | [backward.hpp](../../src/thirdparty/backward-cpp/backward.hpp) |
| brotli | 1.2.1 | Fixed | [version.h](../../src/thirdparty/brotli/common/version.h) |
| chipmunk | 6.1.4 | Screened | [chipmunk.h](../../src/modules/physics/include/eepp/thirdparty/chipmunk/chipmunk.h) |
| cpp-unicodelib | 18.0.0 plus local patch | Screened | [unicodelib.h](../../src/thirdparty/cpp-unicodelib/unicodelib.h) |
| dr_libs | dr_flac 0.13.2; dr_mp3 0.7.2 | Screened | [dr_flac.h](../../src/thirdparty/dr_libs/dr_flac.h) |
| dtl | 1.21 | Screened | [variables.hpp](../../src/thirdparty/dtl/variables.hpp) |
| efsw | 1.7.2 + 5 commits | Screened | [CMakeLists.txt](../../src/thirdparty/efsw/CMakeLists.txt) |
| emscripten-browser-clipboard | eepp-maintained fork (from `633dc92`) | Owned | [emscripten_browser_clipboard.h](../../src/thirdparty/emscripten-browser-clipboard/emscripten_browser_clipboard.h) |
| fast_float | 6.1.6 | Screened | [fast_float.h](../../src/thirdparty/fast_float/include/fast_float/fast_float.h) |
| freetype2 | 2.14.1 | Screened | [freetype.h](../../src/thirdparty/freetype2/include/freetype/freetype.h) |
| fts_fuzzy_match | `d541eb4` plus local patch | Screened | [fts_fuzzy_match.h](../../src/thirdparty/fts_fuzzy_match/fts_fuzzy_match.h) |
| glew | 2.1.0 | Screened | [glew.h](../../src/thirdparty/glew/glew.h) |
| glm | 0.9.4.0 | Screened | [setup.hpp](../../src/thirdparty/glm/core/setup.hpp) |
| gumbo-parser | 0.14.1 | Fixed | [gumbo.h](../../src/thirdparty/gumbo-parser/gumbo.h) |
| harfbuzz | 15.0.0 | Fixed | [hb-version.h](../../src/thirdparty/harfbuzz/hb-version.h) |
| imageresampler | 2.2 | Screened | [resampler.h](../../src/thirdparty/imageresampler/resampler.h) |
| jpeg-compressor | `aeb7d3b` (jpge 1.05) plus local patch | Screened | [jpge.cpp](../../src/thirdparty/jpeg-compressor/jpge.cpp) |
| libogg | 1.3.3 | Screened | [framing.c](../../src/thirdparty/libogg/src/framing.c) |
| libpng | 1.6.59 | Fixed | [png.h](../../src/thirdparty/libpng/png.h) |
| libssh2 | 1.11.2_DEV @ 6e7b2076 + local integration | Fixed | [libssh2.h](../../src/thirdparty/libssh2/include/libssh2.h) |
| libvorbis | 1.3.7 | Fixed | [info.c](../../src/thirdparty/libvorbis/lib/info.c) |
| libwebp | 1.5.0 | Screened | [vp8i_dec.h](../../src/thirdparty/libwebp/src/dec/vp8i_dec.h) |
| libyaml | 0.2.5 | Screened | [config.h](../../src/thirdparty/libyaml/src/config.h) |
| libzip | 1.12.1 | Fixed | [zipconf.h](../../src/thirdparty/libzip/zipconf.h) |
| mbedtls | 3.6.7 | Fixed | [build_info.h](../../src/thirdparty/mbedtls/include/mbedtls/build_info.h) |
| md4c | 0.6.0 | Screened | [md4c.c](../../src/thirdparty/md4c/md4c.c) |
| mojoAL | 2.0.1 | Screened | [mojoal.c](../../src/thirdparty/mojoAL/mojoal.c) |
| mojoAL-SDL3 | 2.9.9 | Screened | [mojoal.c](../../src/thirdparty/mojoAL-SDL3/mojoal.c) |
| monocypher | 4.0.3 | Fixed | [monocypher.h](../../src/thirdparty/monocypher/monocypher.h) |
| nanosvg | Snapshot + 2026-10-25 security patches | Fixed | [nanosvg.h](../../src/thirdparty/nanosvg/nanosvg.h) |
| nlohmann | 3.12.0 | Screened | [json.hpp](../../src/thirdparty/nlohmann/json.hpp) |
| oniguruma | 6.9.10 | Screened | [oniguruma.h](../../src/thirdparty/oniguruma/oniguruma.h) |
| pcre2 | 10.49 | Fixed | [pcre2.h](../../src/thirdparty/pcre2/src/pcre2.h) |
| portable-file-dialogs | eepp-maintained fork (from `7f852d8`) | Owned | [portable-file-dialogs.h](../../src/thirdparty/portable-file-dialogs/portable-file-dialogs.h) |
| pugixml | 1.12 | Screened | [pugixml.hpp](../../src/thirdparty/pugixml/pugixml.hpp) |
| SDL2 | 2.0.12 | Conditional | [SDL_version.h](../../src/thirdparty/SDL2/include/SDL_version.h) |
| SheenBidi | 2.9.0 | Screened | [SBVersion.h](../../src/thirdparty/SheenBidi/Headers/SheenBidi/SBVersion.h) |
| simdutf | 9.2.1 (UTF-8/UTF-32 build) | Fixed | [simdutf.h](../../src/thirdparty/simdutf/simdutf.h) |
| SOIL2 | 1.3.0 snapshot | Screened | [SOIL2.h](../../src/thirdparty/SOIL2/src/SOIL2/SOIL2.h) |
| subprocess | eepp-maintained fork | Owned | [subprocess.h](../../src/thirdparty/subprocess/subprocess.h) |
| tabulate | 1.5.0 | Screened | [tabulate.hpp](../../src/thirdparty/tabulate/tabulate.hpp) |
| tinyexpr | Unversioned/patched snapshot | Fixed | [tinyexpr.c](../../src/thirdparty/tinyexpr/tinyexpr.c) |
| utf8cpp | 4.2.1 | Screened | [utf8.h](../../src/thirdparty/utf8cpp/utf8.h) |
| zlib | 1.3.1 | Screened | [zlib.h](../../src/thirdparty/zlib/zlib.h) |

### Nested and public-header components

| Component | Version | Location/notes |
| --- | --- | --- |
| unordered_dense | 5.0.1 | [unordered_dense.h](../../include/eepp/thirdparty/unordered_dense.h). Public container headers; stl.h and huge_page_allocator.h belong to the same import. |
| svector | 1.4.0 | [svector.h](../../include/eepp/thirdparty/svector.h). Public small-vector header. |
| stb_image | 2.30 | [stb_image.h](../../src/thirdparty/SOIL2/src/SOIL2/stb_image.h). Nested in SOIL2; older 2023 high advisories fixed; CVE-2026-5185 fixed locally in SOIL2 `116dfde`. |
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
- Gumbo, md4c and utf8cpp: see their resolved section; each now matches an upstream release.
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
4. The initial audit performed no dependency edits, exploit execution, new build,
   fuzz campaign or cross-platform runtime validation. The 2026-10-25 follow-up
   upgraded the components in the remediation table and applied the NanoSVG fixes; its
   local and CI validation is recorded above. Earlier Brotli test results are not used
   as evidence for these changes.

Coverage gaps that remain explicit:

- Every import now has an identified upstream base except NanoSVG (FLTK-derived fork) and tinyexpr;
  subprocess, backward-cpp, portable-file-dialogs and emscripten-browser-clipboard are
  eepp-maintained forks, and dr_libs carries per-file versions. Their content is fingerprinted,
  but that does not prove which upstream fixes were imported.
- Database records can be incomplete, disputed or mis-scoped. "Screened" is dated
  evidence, not permanent clearance. The review does not replace code auditing or
  fuzzing for vulnerabilities without public advisories.
- Consumer API searches establish the current repository's callers. They cannot
  prove that downstream eepp users never call a bundled library directly or enable
  a different configuration.
- 64-bit ARM and x86 desktop builds are executed (see validation). Mobile and Emscripten
  builds compile in CI without running tests; 32-bit, OpenSSL and other optional builds
  were assessed from source, not executed. Mbed TLS compiler-dependent issues need per-distribution analysis.

## Remaining implementation batches

1. **Android SDL:** remove the vendored SDL2 2.0.12 copy, download the pinned SDL release before
   the Android build, and port Android to SDL3 (recommended; see "Android SDL2 local changes").
2. Replace the libssh2 development snapshot with a security-fixed stable release
   when one is available, retaining the recorded integration patches.
3. **Mbed TLS 4.x:** migrate from the 3.6 LTS. 4.x removes the classic crypto APIs (entropy,
   `ctr_drbg`, `pk`/`md` contexts) that `MbedTLSSocket` and libssh2's Mbed TLS adapter use, in
   favour of PSA, which now runs in production for TLS 1.3. Drop the local
   `MBEDTLS_THREADING_WIN32` patches if 4.x provides Windows threading.

For subsequent updates, record the upstream release/commit and retained patches
next to the dependency, and regenerate the inventory after each batch. An automated
advisory check needs this provenance plus explicit applicability notes; a scan of
version strings alone would both miss the unversioned imports and recreate the
false positives documented above.

## Periodic review

Review this document every few months, and whenever a dependency is updated:

1. Check each component's upstream for new releases and security advisories (GitHub security
   advisories, release notes, NVD). Rerun the NVD queries listed in the inventory and update
   their retrieval dates.
2. For each new advisory, record whether eepp's configuration and callers reach the affected
   code, as the sections above do, rather than relying on version ranges alone.
3. After any update, record the upstream release or commit, its archive SHA-256 and retained
   local patches, recompute the component's fingerprint, and update the validation results.
4. Bump "Last reviewed" at the top, and remove finished items from the remaining batches.
