with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "gamagora-realtime";

  hardeningDisable = [ "fortify" ];

  NIX_CFLAGS_COMPILE="-isystem glad/include -isystem Cimg/include -isystem tinyply/include";
  buildInputs = [glfw glm cimg x11];
}
