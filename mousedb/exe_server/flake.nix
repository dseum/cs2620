{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      lib = nixpkgs.lib;
    in
    rec {
      packages = lib.attrsets.genAttrs systems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          exe = pkgs.stdenv.mkDerivation {
            name = "exe";
            src = ./.;
            buildInputs = [
              pkgs.cmake
              pkgs.llvmPackages_20.clang-tools
              pkgs.llvmPackages_20.clang
              pkgs.ninja
              pkgs.vcpkg
            ];
            configurePhase = ''
              export CC=${pkgs.llvmPackages_20.clang}/bin/clang
              export CXX=${pkgs.llvmPackages_20.clang}/bin/clang++
              export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
              cmake --preset release
            '';
            buildPhase = ''
              cmake --build --preset release
            '';
            installPhase = ''
              mkdir -p $out/bin
              cp build/exe $out/bin/
            '';
          };
        }
      );
      devShells = lib.attrsets.genAttrs systems (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
        in
        {
          default = pkgs.mkShell {
            buildInputs = [
              pkgs.cmake
              pkgs.llvmPackages_20.clang-tools
              pkgs.llvmPackages_20.clang
              pkgs.ninja
              pkgs.vcpkg
            ];
            shellHook = ''
              export CC=${pkgs.llvmPackages_20.clang}/bin/clang
              export CXX=${pkgs.llvmPackages_20.clang}/bin/clang++
              export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
            '';
          };
        }
      );
      defaultPackage = lib.attrsets.genAttrs systems (system: self.packages.${system}.exe);
    };
}
