{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };
  outputs =
    { self, nixpkgs }:
    let
      system = "aarch64-darwin";
      pkgs = import nixpkgs {
        inherit system;
      };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          apple-sdk_15
        ];
        nativeBuildInputs = with pkgs; [
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
        ];
        shellHook = ''
          export VCPKG_ROOT=${pkgs.vcpkg}/share/vcpkg
          export VCPKG_FORCE_SYSTEM_BINARIES=1
        '';
      };
    };
}
