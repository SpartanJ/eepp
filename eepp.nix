{
  stdenv,
  fetchFromGitHub,
  ninja,
  libx11,
  premake5,
  pkg-config,
  SDL2,
  SDL2_image,
  SDL2_mixer,
  SDL2_ttf,
  openssl,
  zlib,
  libjpeg,
  libpng,
  glew,
  freetype,
  lua,
  patchelf,
}:

let
  # Third-party dependencies
  efsw = fetchFromGitHub {
    owner = "SpartanJ";
    repo = "efsw";
    rev = "22f17a0bcdf3a4edf61f8b14328391463389e548";
    hash = "sha256-BuuQDpqtVMuWrhkNnELnz8JlFGv0fE0Rdtkgad0Wprs=";
  };

  soil2 = fetchFromGitHub {
    owner = "SpartanJ";
    repo = "soil2";
    rev = "e9ae69d4aadaefcca0a7c0ca43bfcee0d4c09b79";
    hash = "sha256-5IB3zoLnqJhWv16YfY6ni+uqbSznsaONyWIhlkNy4uI=";
  };

  premakeNinja = fetchFromGitHub {
    owner = "jimon";
    repo = "premake-ninja";
    rev = "3d214bf1fd7d2c867cdf02b4ab50fb921fdd1155";
    hash = "sha256-0xKwOU7pF9ByNgfsmesQ/+IBw1Z3j3tJJtJnCAP2BBE=";
  };

  premakeCMake = fetchFromGitHub {
    owner = "Jarod42";
    repo = "premake-cmake";
    rev = "8e02bb91a4d0f29d7540de7357574cf3b7c454f9";
    hash = "sha256-lhAIY4FUzM+xMTjGVKvcEVDUjYozRfachYSiSkoPwC8=";
  };

  # Architecture-specific variables
  archConfig = if stdenv.isAarch64 then "release_arm64" else "release_x86_64";
  archLibDir = if stdenv.isAarch64 then "libs/linux/aarch64" else "libs/linux/x86_64";
in

stdenv.mkDerivation {
  pname = "eepp";
  version = "unstable";
  src = ./.;

  nativeBuildInputs = [
    premake5
    pkg-config
    patchelf
  ];

  buildInputs = [
    libx11
    SDL2
    SDL2_image
    SDL2_mixer
    SDL2_ttf
    openssl
    zlib
    libjpeg
    libpng
    glew
    freetype
    lua
  ];

  configurePhase = ''
    rm -rf src/thirdparty/efsw
    rm -rf src/thirdparty/SOIL2
    cp -rp ${efsw} src/thirdparty/efsw
    cp -rp ${soil2} src/thirdparty/SOIL2

    rm -rf premake/premake-ninja
    rm -rf premake/premake-cmake
    cp -rp ${premakeNinja} premake/premake-ninja
    cp -rp ${premakeCMake} premake/premake-cmake

    premake5 gmake
  '';

  buildPhase = ''
    make -C make/linux config=${archConfig} -j$(nproc)
  '';

  installPhase = ''
    mkdir -p $out
    cp -R ${archLibDir}/ $out/lib
    cp -R bin $out/bin

    find "$out/bin" -type l -lname '/build/*' -delete

    find "$out/bin" -type f -executable -exec sh -c '
      file "$1" | grep -q ELF && patchelf --add-rpath "'"$out/lib"'" "$1"
    ' _ {} \;
  '';
}
