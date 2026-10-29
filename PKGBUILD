# Maintainer: Alexander Tsidaev <far2l@tsidaev.ru>
# Packages the tree produced by `make release` (run that first, or use `make zst`).

pkgname=far2l-plugins-speccy
pkgver=0.9
pkgrel=1
pkgdesc='far2l plugins for ZX Spectrum formats (TR-DOS, IS-DOS, HRiP, ZXZip, xLook)'
arch=('x86_64')
url='https://github.com/atsidaev/far2l-plugins-speccy'
license=('custom')
depends=('far2l')
options=('!strip' '!debug')
source=()
sha256sums=()

package() {
  local plugins="$startdir/release/usr/lib/far2l/Plugins"
  if [[ ! -d $plugins ]]; then
    error 'Missing release/usr/lib/far2l/Plugins — run `make release` (or `make zst`) first'
    return 1
  fi
  install -d "$pkgdir/usr/lib/far2l/Plugins"
  cp -a "$plugins"/. "$pkgdir/usr/lib/far2l/Plugins/"
}
