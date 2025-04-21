FROM nixos/nix:latest

RUN echo "experimental-features = nix-command flakes" >> /etc/nix/nix.conf

WORKDIR /code
COPY . .

RUN nix build

ENTRYPOINT ["nix", "develop", "--command"]
CMD ["bash"]
