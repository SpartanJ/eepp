{
  description = "A flake for building eepp projects";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    { self, nixpkgs }:
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
            in
            {
              eepp = pkgs.callPackage ./eepp.nix { };
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
