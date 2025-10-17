{ stdenv, cmake, pkg-config, libsForQt5, ... }:

stdenv.mkDerivation rec {
  pname = "qcomicbook";
  version = "0.9.1";

  src = ./.;

  nativeBuildInputs = [
    cmake
    pkg-config
    libsForQt5.wrapQtAppsHook
  ];

  buildInputs = [
    libsForQt5.qtbase
    libsForQt5.qttools
    libsForQt5.qtx11extras
    libsForQt5.poppler
  ];

  postInstall = ''
    substituteInPlace $out/share/applications/*.desktop \
      --replace "Exec=qcomicbook" "Exec=$out/bin/qcomicbook"
  '';

}
