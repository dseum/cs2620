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
        "aarch64-darwin"
        "x86_64-linux"
        "aarch64-linux"
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
                (if pkgs.stdenv.isDarwin then apple-sdk_15 else null)
              ]
              ++ (with pkgs; [
                autoconf
                autoconf-archive
                automake
                cmake
                git
                llvmPackages_20.clang-tools
                llvmPackages_20.clangWithLibcAndBasicRtAndLibcxx
                ninja
                pkg-config
                vcpkg
              ]);

            shellHook = ''
              export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
              export VCPKG_FORCE_SYSTEM_BINARIES=1
            '';
          };
        }
      );
}
