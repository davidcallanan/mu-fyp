# As I have recently moved from Windows to NixOS after my laptop went kapoosh, this file sets up the necessary environment for development.

{ pkgs ? import <nixpkgs> {} }:

(pkgs.buildFHSEnv {
  name = "dev-environment";
  targetPkgs = pkgs: (with pkgs; [
    nodejs_20
    nodePackages.pnpm
    bashInteractive
  ]);
  
  runScript = "bash";
}).env

# We launch this environment by typing `nix-shell shell.nix`.

# Note that it is my understanding that this setup mirrors the home directory of the system and the environment, so it pollutes the home directory.
# Not enough of a concern for me at this moment.
