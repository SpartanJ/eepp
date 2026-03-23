{
  description = "A flake for building eepp projects";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    efsw = {
      url = "github:SpartanJ/efsw/22f17a0bcdf3a4edf61f8b14328391463389e548";
      flake = false;
    };
    soil2 = {
      url = "github:SpartanJ/SOIL2/e9ae69d4aadaefcca0a7c0ca43bfcee0d4c09b79";
      flake = false;
    };
    premakeNinja = {
      url = "github:jimon/premake-ninja/3d214bf1fd7d2c867cdf02b4ab50fb921fdd1155";
      flake = false;
    };
    premakeCMake = {
      url = "github:Jarod42/premake-cmake/8e02bb91a4d0f29d7540de7357574cf3b7c454f9";
      flake = false;
    };
  };

  outputs =
    {
      self,
      efsw,
      soil2,
      premakeNinja,
      premakeCMake,
      nixpkgs,
    }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
    in
    {
      # For each supported system, define packages and defaultPackage
      packages = builtins.listToAttrs (
        map (system: {
          name = system;
          value =
            let
              pkgs = import nixpkgs { inherit system; };
              eepp_pkgs = (
                {
                  stdenv,
                  efsw,
                  soil2,
                  premakeNinja,
                  premakeCMake,
                  ninja,
                  glew,
                  libx11,
                  SDL2,
                  premake5,
                  pkg-config,
                  patchelf,
                }:

                let
                  # Architecture-specific variables
                  archConfig = if stdenv.isAarch64 then "release_arm64" else "release_x86_64";
                  archLibDir = if stdenv.isAarch64 then "libs/linux/aarch64" else "libs/linux/x86_64";
                in

                stdenv.mkDerivation {
                  pname = "eepp";
                  version = "unstable";
                  src = self;

                  nativeBuildInputs = [
                    premake5
                    pkg-config
                    patchelf
                  ];

                  buildInputs = [
                    glew
                    SDL2
                    libx11
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

                    premake5 --disable-static-build gmake
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
              );
            in
            {
              eepp = pkgs.callPackage eepp_pkgs {
                efsw = efsw;
                soil2 = soil2;
                premakeNinja = premakeNinja;
                premakeCMake = premakeCMake;
              };
            };
        }) systems
      );

      defaultPackage = builtins.listToAttrs (
        map (system: {
          name = system;
          value = self.packages.${system}.eepp;
        }) systems
      );
    };
}
