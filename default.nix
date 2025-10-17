{ pkgs ? import <nixpkgs> {} }:

pkgs.callPackage (
let
  inherit (pkgs) stdenv cmake pkg-config libsForQt5 lib;
in
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

  meta = with lib; {
    homepage = "https://github.com/stolowski/QComicBook";
    description = "Comic book reader in Qt5";
    mainProgram = "qcomicbook";
    license = licenses.gpl2;

    longDescription = ''
      QComicBook is a viewer for PDF files and comic book archives containing
      jpeg/png/xpm/gif/bmp images, which aims at convenience and simplicity.
      Features include: automatic unpacking of archive files, full-screen mode, continuous
      scrolling mode, double-pages viewing, manga mode, thumbnails view, page scaling,
      mouse or keyboard navigation etc.
    '';

    platforms = platforms.linux;
    maintainers = with maintainers; [ greydot ];
  };
}) {}

