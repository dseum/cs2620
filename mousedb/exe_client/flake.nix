{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
    in
    {
      packages = builtins.listToAttrs (
        map (system: {
          name = system;
          value = import nixpkgs { system = system; };
        }) systems
      );
      devShells = builtins.listToAttrs (
        map (system: {
          name = system;
          value =
            let
              pkgs = import nixpkgs { system = system; };
            in
            pkgs.mkShell { buildInputs = [ pkgs.clang ]; };
        }) systems
      );
      apps = builtins.listToAttrs (
        map (system: {
          name = system;
          value =
            let
              pkgs = import nixpkgs { system = system; };
              src = pkgs.writeTextDir "example" {
                main.c = ''
                  #include <stdio.h>
                  int main(void) { printf("Hello, Nix!"); return 0; }
                '';
              };
              deriv = pkgs.stdenv.mkDerivation {
                name = "hello-clang";
                inherit src;
                buildInputs = [ pkgs.clang ];
                buildPhase = "clang main.c -o hello";
                installPhase = "mkdir -p $out/bin && cp hello $out/bin/";
              };
            in
            {
              type = "app";
              program = "${deriv}/bin/hello";
            };
        }) systems
      );
    };
}
