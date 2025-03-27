{
  description = "A very basic Zephyr flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";

    # Customize the version of Zephyr used by the flake here
    zephyr.url = "github:zephyrproject-rtos/zephyr/v4.1.0";
    zephyr.flake = false;

    zephyr-nix.url = "github:dvalnn/zephyr-nix";
    zephyr-nix.inputs.nixpkgs.follows = "nixpkgs";
    zephyr-nix.inputs.zephyr.follows = "zephyr";
  };

  outputs =
    {
      self,
      nixpkgs,
      zephyr-nix,
      ...
    }:
    let
      pkgs = nixpkgs.legacyPackages.x86_64-linux;
      zephyr = zephyr-nix.packages.x86_64-linux;
    in
    {
      devShells.x86_64-linux.default = pkgs.mkShell {
        packages = with pkgs; [
          (zephyr.sdk.override {
            targets = [
              "x86_64-zephyr-elf"
              "arm-zephyr-eabi"
            ];
          })
          zephyr.pythonEnv
          # Use zephyr.hosttools-nix to use nixpkgs built tooling instead of official Zephyr binaries
          zephyr.hosttools
          cmake
          ninja

          # Project specific dependencies
          clang-tools # for clangd
          glibc_multi
        ];

        shellHook = ''
          export ZEPHYR_BASE=$(pwd)/zephyr
        '';
      };
    };
}
