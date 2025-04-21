FROM nixos/nix:latest

RUN echo "experimental-features = nix-command flakes" >> /etc/nix/nix.conf

WORKDIR /code

COPY flake.nix flake.lock exe_server/
COPY exe_server/flake.nix exe_server/flake.lock exe_server/

WORKDIR /code/exe_server

RUN nix develop --command true

COPY . . 

ENTRYPOINT ["nix", "develop"]
