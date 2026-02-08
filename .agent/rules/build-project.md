---
trigger: always_on
---

To build the project in debug mode you must run from the root project directory:

`make -C make/linux -j$(nproc)`

If any file has been added you should also run (previous to the make command):

`premake4 --disable-static-build --with-mold-linker --with-debug-symbols --address-sanitizer gmake`