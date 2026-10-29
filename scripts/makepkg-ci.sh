#!/usr/bin/env bash
# Build Arch package inside CI (archlinux container / docker).
set -euo pipefail

: "${VERSION:?VERSION is required}"

if [[ ! -d release/usr/lib/far2l/Plugins ]]; then
  echo "Missing release/usr/lib/far2l/Plugins — run make release/zip/deb first" >&2
  exit 1
fi
if [[ ! -f PKGBUILD ]]; then
  echo "Missing PKGBUILD" >&2
  exit 1
fi

pacman -Syu --noconfirm --needed base-devel

if ! id builder >/dev/null 2>&1; then
  useradd -m builder
  echo 'builder ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers
fi

chown -R builder:builder .

sudo -u builder env VERSION="$VERSION" bash -lc '
  set -euo pipefail
  sed -i "s/^pkgver=.*/pkgver=${VERSION}/" PKGBUILD
  rm -rf pkg src
  makepkg -f
  test -f "far2l-plugins-speccy-${VERSION}-1-x86_64.pkg.tar.zst"
'
