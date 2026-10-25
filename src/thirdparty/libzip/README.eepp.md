# libzip 1.12.1

Source: `lib/` of the libzip 1.12.1 release (https://libzip.org,
`libzip-1.12.1.tar.xz`, SHA-256
`c40062022a1963c80573d48f6070792297cecd7c6bffe3aefcac0d8b9bee8913`), BSD-3-Clause, see `LICENSE`.

Only the sources upstream's `lib/CMakeLists.txt` builds for deflate/store archives are imported.
bzip2, xz, zstd and WinZip AES (`HAVE_CRYPTO`) support are not built; traditional PKWARE
encryption is. The platform file and random backends are split into `posix/` and `win32/` so
build scripts can select them by directory; they require `src/thirdparty/libzip` on the
include path.

## Generated and eepp files

- `config.h` and `zipconf.h` replace the files CMake generates; they hold the feature checks for
  every eepp platform. `ZIP_STATIC` is always defined.
- `zip_err_str.c` is generated from `zip.h` and `zipint.h` with upstream's
  `cmake/GenerateZipErrorStrings.cmake`. Regenerate it when updating:
  `cmake -DPROJECT_SOURCE_DIR=<dir with lib/zip.h and lib/zipint.h> -DCMAKE_CURRENT_BINARY_DIR=<out> -P GenerateZipErrorStrings.cmake`.

## Local patches

Every other imported file is unmodified.

- `zip.h` includes `"zipconf.h"` instead of `<zipconf.h>` (marked `eepp:`), so eepp includes
  `<libzip/zip.h>` without adding `src/thirdparty/libzip` to every project's include path.

When updating, re-apply this patch and regenerate `zip_err_str.c`.
