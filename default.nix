with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "HRLZ";
  nativeBuildInputs = with pkgs; [ cmake gcc11 ];
  src = ./.;
  installPhase = ''
    mkdir -p $out/bin
    find apps -executable -type f -exec cp "{}" $out/bin \;
    '';
}
