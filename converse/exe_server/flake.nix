{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachSystem
      [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-linux"
        "aarch64-darwin"
      ]
      (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };
        in
        {
          devShells.default = pkgs.mkShell {
            buildInputs =
              with pkgs;
              [
                autoconf
                autoconf-archive
                automake
                cmake
                git
                llvmPackages_20.clang-tools
                llvmPackages_20.clangWithLibcAndBasicRtAndLibcxx
                ninja
                perl
                pkg-config
                vcpkg
              ]
              ++ lib.optionals stdenv.isDarwin [
                apple-sdk_15
              ];
            shellHook = ''
              export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
              export VCPKG_FORCE_SYSTEM_BINARIES=1
              export CC=${pkgs.llvmPackages_20.clangWithLibcAndBasicRtAndLibcxx}/bin/clang
              export CXX=${pkgs.llvmPackages_20.clangWithLibcAndBasicRtAndLibcxx}/bin/clang++
            '';
          };
        }
      );
}
